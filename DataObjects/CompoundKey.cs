public enum KeyType : byte
{
    PostById = 0,
    CommentById = 1,
    CommentsByPost = 2,
    PostsByTag = 3,
    TagInfo = 4
}

public enum SiteID : ushort
{
    AskUbuntu = 1
}

public struct CompoundKey
{
    public byte KeyType;
    public ushort SiteId;
    public uint PrimaryId;
    public byte Reserved;

    public CompoundKey(byte keyType, ushort siteId, uint primaryId, byte reserved = 0)
    {
        KeyType = keyType;
        SiteId = siteId;
        PrimaryId = primaryId;
        Reserved = reserved;
    }

    // 8b Type | 16b Site | 32b PrimaryID | 8b Reserved
    public ulong Pack()
    {
        return ((ulong)KeyType << 56) |
               ((ulong)SiteId << 40) |
               ((ulong)PrimaryId << 8) |
               (ulong)Reserved;
    }

    public static CompoundKey Unpack(ulong packed)
    {
        return new CompoundKey(
            (byte)(packed >> 56),
            (ushort)(packed >> 40),
            (uint)(packed >> 8),
            (byte)(packed & 0xFF)
        );
    }

    public override string ToString()
    {
        string typeStr = Enum.IsDefined(typeof(KeyType), KeyType) 
            ? ((KeyType)KeyType).ToString() 
            : $"UNKNOWN({KeyType})";

        string siteStr = Enum.IsDefined(typeof(SiteID), SiteId) 
            ? ((SiteID)SiteId).ToString() 
            : $"UNKNOWN({SiteId})";

        return $"KeyType:{typeStr} Site:{siteStr} ID:{PrimaryId} (packed:0x{Pack():x})";
    }
}