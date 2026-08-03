open System
open System.IO
open System.Collections.Generic
open System.Text
open System.Security.Cryptography

type XboxCikEntry(bytes: byte[]) =
    member val Id = new Guid(bytes.[..0xF])
    member val TweakKey = bytes.[0x10..0x1F]
    member val DataKey = bytes.[0x20..]

let GenerateXboxCikEntriesHeader () =

    let cikDirectory = Path.Combine(Directory.GetCurrentDirectory(), "Xbox/Keys/Ciks")
    let cikEntries = new List<XboxCikEntry>()

    try
        for cikFile in Directory.GetFiles(cikDirectory, "*.cik", SearchOption.TopDirectoryOnly) do

            let cikContents = File.ReadAllBytes(cikFile)
            if cikContents.Length <> 0x30 then
                failwith "Invalid cik length"

            let cikEntry = new XboxCikEntry(cikContents) 
            if cikEntry.Id = Guid.Empty then
                failwith "Invalid cik id"

            cikEntries.Add(cikEntry)

    with
        | :? DirectoryNotFoundException -> ()
        | ex -> raise(ex)
    
    let bytesToHex (bytes: byte[]) = String.Join(", ", bytes |> Array.map (fun b -> $"0x{b:X2}"))

    let headerBuilder = new StringBuilder(2048)
    headerBuilder
        .AppendLine("#pragma once")
        .AppendLine()
        .AppendLine("namespace Citrine::Xbox {")
        .AppendLine()
        .AppendLine("\tconsteval auto GetCikEntries() noexcept -> auto {")
        .AppendLine()
        .AppendLine($"\t\treturn std::array<CikEntry, {cikEntries.Count}>{{{{")
        |> ignore

    cikEntries |> Seq.iteri (fun index cikEntry ->
    
        headerBuilder
            .AppendLine("\t\t\t{") 
            .AppendLine($"\t\t\t\t\"{cikEntry.Id}\"_Guid,")
            .AppendLine($"\t\t\t\t{{ {bytesToHex cikEntry.TweakKey} }},")
            .AppendLine($"\t\t\t\t{{ {bytesToHex cikEntry.DataKey} }}")
            .AppendLine(if index < cikEntries.Count - 1 then "\t\t\t}," else "\t\t\t}")
            |> ignore
    )

    headerBuilder
        .AppendLine("\t\t}};")
        .AppendLine("\t}")
        .AppendLine("}")
        |> ignore

    headerBuilder.ToString()

let GenerateWUProtocolTemplatesHeader () =
    
    let headerBuilder = new StringBuilder(2048)
    headerBuilder
        .AppendLine("#pragma once")
        .AppendLine()
        .AppendLine("namespace Citrine::Windows {")
        .AppendLine()
        .AppendLine("\tstruct WUProtocolTemplates {")
        |> ignore

    let templateDirectory = Path.Combine(Directory.GetCurrentDirectory(), "Windows/WUProtocol")

    try
        for templateFile in Directory.GetFiles(templateDirectory, "*.xml", SearchOption.TopDirectoryOnly) do

            let templateName = Path.GetFileNameWithoutExtension(templateFile)
            let templateContent = File.ReadAllText(templateFile)

            headerBuilder
                .AppendLine()
                .AppendLine($"\t\tstatic constexpr auto& {templateName}{{")
                .AppendLine($"R\"({templateContent})\"")
                .AppendLine("\t\t};")
                |> ignore

    with
        | :? DirectoryNotFoundException -> ()
        | ex -> raise(ex)

    headerBuilder
        .AppendLine("\t};")
        .AppendLine("}")
        |> ignore

    headerBuilder.ToString()

let WriteHeaderFile (path: string, content: string) =
    
    let contentBytes = Encoding.UTF8.GetBytes(content);
    use fileStream = new FileStream(path, FileMode.OpenOrCreate, FileAccess.ReadWrite)
    
    use sha256 = SHA256.Create()
    let oldContentHash = sha256.ComputeHash(fileStream);
    let newContentHash = sha256.ComputeHash(contentBytes);

    if oldContentHash <> newContentHash then
        fileStream.Seek(0, SeekOrigin.Begin)        |> ignore
        fileStream.Write(contentBytes)              |> ignore
        fileStream.SetLength(contentBytes.Length)   |> ignore

let generatedFilesDirectory = Path.Combine(Directory.GetCurrentDirectory(), "Generated Files");
Directory.CreateDirectory(generatedFilesDirectory);
WriteHeaderFile(Path.Combine(generatedFilesDirectory, "XboxCikEntries.h"), GenerateXboxCikEntriesHeader())
WriteHeaderFile(Path.Combine(generatedFilesDirectory, "WUProtocolTemplates.h"), GenerateWUProtocolTemplatesHeader())
