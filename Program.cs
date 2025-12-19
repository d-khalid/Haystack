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
        // 1. If no args, run Web API mode
        if (args.Length == 0)
        {
            return RunWebApi(args);
        }

        // 2. Otherwise, run CLI mode
        return await RunCli(args);
    }

    private static int RunWebApi(string[] args)
    {
        var builder = WebApplication.CreateBuilder(args);
        builder.Services.AddControllers();
        
        // Register your components for the API to use
        builder.Services.AddSingleton<Lexicon>();
        // builder.Services.AddSingleton<QueryProcessor>();

        var app = builder.Build();
        app.MapControllers();
        app.Run();
        return 0;
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