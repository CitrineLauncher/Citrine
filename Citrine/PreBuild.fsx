open System
open System.IO
open System.Collections.Generic
open System.Text

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

let generatedFilesDirectory = Path.Combine(Directory.GetCurrentDirectory(), "Generated Files");
Directory.CreateDirectory(generatedFilesDirectory);
File.WriteAllText(Path.Combine(generatedFilesDirectory, "XboxCikEntries.h"), GenerateXboxCikEntriesHeader())
