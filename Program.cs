using System;
using System.CommandLine;
using System.CommandLine.Invocation;
using System.IO;
using System.Threading.Tasks;
using Microsoft.AspNetCore.Builder;
using Microsoft.Extensions.DependencyInjection;

// This program functions as both a generating CLI and as a web API for the backend
// Depending on the CLI args
public class Program
{
    public static async Task<int> Main(string[] args)
    {
        // 1. If single argument that is a valid directory, run Web API mode with that directory
        if (args.Length == 1 && Directory.Exists(args[0]))
        {
            return RunWebApi(new[] { args[0] });
        }

        // 2. If no args, run Web API mode with default directory
        if (args.Length == 0)
        {
            return RunWebApi(args);
        }

        // 3. Otherwise, run CLI mode
        return await RunCli(args);
    }

    private static int RunWebApi(string[] args)
    {
        var builder = WebApplication.CreateBuilder(args);
        builder.Services.AddControllers();

        // Configure CORS to allow frontend access
        builder.Services.AddCors(options =>
        {
            options.AddPolicy("AllowFrontend", corsBuilder =>
            {
                corsBuilder.WithOrigins("http://localhost:4200", "http://20.255.48.227")
                           .AllowAnyMethod()
                           .AllowAnyHeader();
            });
        });
        
        // Determine data directory: use command line argument if provided, otherwise use config or default
        string dataDir;
        if (args.Length > 0 && Directory.Exists(args[0]))
        {
            dataDir = Path.GetFullPath(args[0]);
            Console.WriteLine($"[API] Using data directory from command line argument: {dataDir}");
        }
        else
        {
            var configuration = builder.Configuration;
            string configDir = configuration.GetSection("DataDirectory").Value ?? "stackexchange-data";
            dataDir = Path.GetFullPath(configDir);
            Console.WriteLine($"[API] Using data directory from configuration: {dataDir}");
        }

        // Verify directory exists
        if (!Directory.Exists(dataDir))
        {
            Console.WriteLine($"[API] ERROR: Data directory does not exist: {dataDir}");
            Console.WriteLine($"[API] Current working directory: {Directory.GetCurrentDirectory()}");
            return 1;
        }

        Console.WriteLine($"[API] Data directory verified: {dataDir}");
        Console.WriteLine($"[API] Files in directory: {string.Join(", ", Directory.GetFiles(dataDir))}");

        int numBarrels = 4; // Default to 4 barrels
        try
        {
            var configuration = builder.Configuration;
            numBarrels = int.Parse(configuration.GetSection("NumBarrels").Value ?? "4");
        }
        catch
        {
            Console.WriteLine("[API] Could not parse NumBarrels from configuration, using default of 4");
        }

        // Register components for the API to use
        builder.Services.AddSingleton<Lexicon>(sp =>
        {
            var lexicon = new Lexicon();
            var lexiconPath = Path.Combine(dataDir, "lexicon.txt");
            if (File.Exists(lexiconPath))
            {
                Console.WriteLine($"[API] Loading lexicon from: {lexiconPath}");
                lexicon.Load(lexiconPath);
                Console.WriteLine($"[API] Lexicon loaded successfully with {lexicon.Count} words");
            }
            else
            {
                Console.WriteLine($"[API] ERROR: Lexicon file not found at: {lexiconPath}");
            }
            return lexicon;
        });

        builder.Services.AddSingleton<AutocompleteService>(sp =>
        {
            var autocompleteService = new AutocompleteService();
            var vocabPath = Path.Combine(dataDir, "vocab.txt");
            if (File.Exists(vocabPath))
            {
                Console.WriteLine($"[API] Loading vocabulary from: {vocabPath}");
                autocompleteService.LoadFromFile(vocabPath);
                Console.WriteLine($"[API] Vocabulary loaded successfully");
            }
            else
            {
                Console.WriteLine($"[API] ERROR: Vocabulary file not found at: {vocabPath}");
            }
            return autocompleteService;
        });

        builder.Services.AddSingleton<ISAMStorage>(sp =>
        {
            var dataIndexPath = Path.Join(dataDir, "data_index");
            Console.WriteLine($"[API] Initializing ISAM storage with base path: {dataIndexPath}");
            return new ISAMStorage(dataIndexPath);
        });

        builder.Services.AddSingleton<QueryEngine>(sp =>
        {
            var lexicon = sp.GetRequiredService<Lexicon>();
            var dataIndex = sp.GetRequiredService<ISAMStorage>();
            Console.WriteLine($"[API] Initializing QueryEngine with {numBarrels} barrels and directory: {dataDir}");
            return new QueryEngine(dataDir, lexicon, numBarrels, dataIndex);
        });

        var app = builder.Build();
        app.MapControllers();

        // Enable CORS
        app.UseCors("AllowFrontend");

        // Start background task to flush delta index every 2 hours
        var queryEngine = app.Services.GetRequiredService<QueryEngine>();
        _ = Task.Run(() => StartFlushBackgroundTask(queryEngine));

        app.Run();
        return 0;
    }

    /// <summary>
    /// Background task that periodically flushes the delta index to disk.
    /// Runs every 2 hours to persist newly added posts.
    /// </summary>
    private static async Task StartFlushBackgroundTask(QueryEngine queryEngine)
    {
        const int flushIntervalMs = 2 * 60 * 60 * 1000; // 2 hours in milliseconds
        
        while (true)
        {
            try
            {
                await Task.Delay(flushIntervalMs);
                Console.WriteLine($"[{DateTime.Now:yyyy-MM-dd HH:mm:ss}] Starting background flush of delta index...");
                queryEngine.FlushDeltaToDisk();
                Console.WriteLine($"[{DateTime.Now:yyyy-MM-dd HH:mm:ss}] Delta index flush completed successfully.");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[{DateTime.Now:yyyy-MM-dd HH:mm:ss}] Error during delta index flush: {ex.Message}");
            }
        }
    }

    private static async Task<int> RunCli(string[] args)
    {
        var rootCommand = new RootCommand("The Haystack CLI - Search Engine Management");

        // Common Options
        var inputOpt = new Option<DirectoryInfo>("--input", "Input directory containing data").ExistingOnly();
        inputOpt.Required = true;
        inputOpt.AddAlias("-i");

        var barrelOpt = new Option<int>("--barrels", () => 1, "Number of barrels");
        barrelOpt.AddAlias("-b");

        // Commands for indexing 
        var indexCmd = new Command("index", "Generation and indexing operations");
        var genData = new Option<bool>("--data-index", "Generate data index");
        var genLexicon = new Option<bool>("--lexicon", "Generate lexicon + autocomplete trie");
        var genForward = new Option<bool>("--forward", "Generate forward index");
        var genReverse = new Option<bool>("--reverse", "Generate reverse index");
        
        indexCmd.AddOption(inputOpt);
        indexCmd.AddOption(genData);
        indexCmd.AddOption(genLexicon);
        indexCmd.AddOption(genForward);
        indexCmd.AddOption(genReverse);
        indexCmd.AddOption(barrelOpt);

        
        
        indexCmd.Handler = CommandHandler.Create<DirectoryInfo, bool, bool, bool, bool, int>(
            (input, dataIndex, lexicon, forward, reverse, barrels) => 
            {
                Console.WriteLine($"Indexing Directory: {input.FullName}");

                if (dataIndex)
                {
                    Console.WriteLine("Action: Generating Data index...");

                    // using syntax automatically disposes
                    using ISAMStorage dIndexIsam = new ISAMStorage(Path.Join(input.FullName, "data_index"));
                    DataIndex.Generate(dIndexIsam, Path.Join(input.FullName, "Posts.xml"), SiteID.AskUbuntu);
                    
                }
                
                if (lexicon) {
                    Console.WriteLine("Action: Generating Lexicon+Trie...");
                    using ISAMStorage d = new ISAMStorage(Path.Join(input.FullName, "data_index"));
                    
                    (Lexicon, AutocompleteService) pair = Lexicon.Generate(d);
                    pair.Item1.Save(Path.Join(input.FullName, "lexicon.txt"));
                    pair.Item2.ExportToFile(Path.Join(input.FullName, "vocab.txt")); //
                    
                }
                if (forward) {
                    Console.WriteLine("Action: Generating Forward Index...");
                    
                    using ISAMStorage fIndex = new ISAMStorage(Path.Join(input.FullName, "forward_index"));
                    using ISAMStorage dIndex = new ISAMStorage(Path.Join(input.FullName, "data_index"));
                    Lexicon l = new Lexicon();
                    l.Load(Path.Join(input.FullName, "lexicon.txt"));
                    
                    ForwardIndex.Generate(fIndex, dIndex, l);
                }
                if (reverse) {
                    Console.WriteLine($"Action: Generating Reverse Index with {barrels} barrels...");
                    
                    using ISAMStorage f = new ISAMStorage(Path.Join(input.FullName, "forward_index"));
                    
                    InvertedIndex.BuildAndSave(f, input.FullName, barrels);
                }
            });

        // Commands for search
        var searchCmd = new Command("search", "Search the index");
        var queryOpt = new Option<string>("--query", "Text query to search");
        var idOpt = new Option<int>("--id", "Search by WordID");
        var autoOpt = new Option<string>("--autocomplete", "Prefix for autocomplete");

        searchCmd.AddOption(inputOpt);
        searchCmd.AddOption(queryOpt);
        searchCmd.AddOption(idOpt);
        searchCmd.AddOption(autoOpt);
        searchCmd.AddOption(barrelOpt);

        searchCmd.Handler = CommandHandler.Create<DirectoryInfo, string, int, string, int>(
            (input, query, id, autocomplete, barrels) => 
            {
                if (!string.IsNullOrEmpty(query)) {
                    Console.WriteLine($"Searching for text: {query}");
                    // var qp = new QueryProcessor(input.FullName, barrels);
                    // qp.Search(query);
                }
        
                if (id > 0) {
                    Console.WriteLine($"Searching for Word ID: {id}");
                }

                if (!string.IsNullOrEmpty(autocomplete)) {
                    Console.WriteLine($"Running autocomplete for: {autocomplete}");
                }
            });

        // Commands for adding more 
        var addCmd = new Command("add", "Add manual documents");
        var jsonOpt = new Option<FileInfo>("--json", "Add from JSON file").ExistingOnly();

        addCmd.AddOption(inputOpt);
        addCmd.AddOption(jsonOpt);

        
       
        addCmd.Handler = CommandHandler.Create<DirectoryInfo, FileInfo>(
            (input, json) => 
            {
                if (json != null) {
                    Console.WriteLine($"Adding documents from JSON: {json.FullName}");
                }
                
            });

        rootCommand.AddCommand(indexCmd);
        rootCommand.AddCommand(searchCmd);
        rootCommand.AddCommand(addCmd);

        return await rootCommand.InvokeAsync(args);
    }
}