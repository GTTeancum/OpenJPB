// Decompile exact PDB-mapped functions without running whole-program analysis.
// @category JediPowerBattles

import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;

import ghidra.app.cmd.disassemble.DisassembleCommand;
import ghidra.app.cmd.function.CreateFunctionCmd;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.SourceType;

public class DecompileMappedRange extends GhidraScript {
    private static final int DECOMPILE_TIMEOUT_SECONDS = 180;

    private static final class MappedFunction {
        long address;
        long size;
        String name;
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 4) {
            printerr(
                "Usage: DecompileMappedRange.java <start> <end> " +
                "<output-dir> <function-map>");
            return;
        }

        long startValue = Long.decode(args[0]);
        long endValue = Long.decode(args[1]);
        Path outputDirectory =
            Paths.get(args[2]).toAbsolutePath().normalize();
        Path functionMap = Paths.get(args[3]).toAbsolutePath().normalize();
        Files.createDirectories(outputDirectory);

        List<MappedFunction> mappedFunctions = new ArrayList<>();
        for (String line : Files.readAllLines(
                functionMap, StandardCharsets.UTF_8)) {
            if (line.isBlank() || line.startsWith("va\t")) {
                continue;
            }
            String[] fields = line.split("\t", -1);
            if (fields.length < 6) {
                continue;
            }
            long address = Long.decode(fields[0]);
            if (address < startValue || address > endValue) {
                continue;
            }

            MappedFunction mappedFunction = new MappedFunction();
            mappedFunction.address = address;
            mappedFunction.size = Long.decode(fields[2]);
            mappedFunction.name = fields[5];
            mappedFunctions.add(mappedFunction);
        }
        mappedFunctions.sort(Comparator.comparingLong(item -> item.address));

        DecompInterface decompiler = new DecompInterface();
        decompiler.toggleCCode(true);
        decompiler.toggleSyntaxTree(true);
        try {
            for (MappedFunction mappedFunction : mappedFunctions) {
                Address entry = toAddr(mappedFunction.address);
                Address bodyEnd = entry.add(mappedFunction.size - 1);
                AddressSet body = new AddressSet(entry, bodyEnd);

                DisassembleCommand disassemble =
                    new DisassembleCommand(entry, body, true);
                disassemble.enableCodeAnalysis(false);
                if (!disassemble.applyTo(currentProgram, monitor)) {
                    printerr(
                        "Disassembly failed at " + entry + ": " +
                        disassemble.getStatusMsg());
                    continue;
                }

                Function function = currentProgram
                    .getFunctionManager().getFunctionAt(entry);
                if (function == null) {
                    CreateFunctionCmd create = new CreateFunctionCmd(
                        mappedFunction.name,
                        entry,
                        body,
                        SourceType.IMPORTED);
                    if (!create.applyTo(currentProgram, monitor)) {
                        printerr("Function creation failed at " + entry);
                        continue;
                    }
                    function = create.getFunction();
                }
                else {
                    function.setName(
                        mappedFunction.name, SourceType.IMPORTED);
                }
            }

            if (!decompiler.openProgram(currentProgram)) {
                throw new IllegalStateException(
                    "Could not initialize decompiler: " +
                    decompiler.getLastMessage());
            }

            int completed = 0;
            int failed = 0;
            for (MappedFunction mappedFunction : mappedFunctions) {
                Address entry = toAddr(mappedFunction.address);
                Function function = currentProgram
                    .getFunctionManager().getFunctionAt(entry);
                if (function == null) {
                    failed++;
                    continue;
                }

                DecompileResults result = decompiler.decompileFunction(
                    function, DECOMPILE_TIMEOUT_SECONDS, monitor);
                if (!result.decompileCompleted() ||
                        result.getDecompiledFunction() == null) {
                    printerr(
                        "Decompilation failed at " + entry + ": " +
                        result.getErrorMessage());
                    failed++;
                    continue;
                }

                String fileName = entry.toString() + "_" +
                    mappedFunction.name.replaceAll(
                        "[^A-Za-z0-9_.-]", "_") + ".c";
                Files.writeString(
                    outputDirectory.resolve(fileName),
                    result.getDecompiledFunction().getC(),
                    StandardCharsets.UTF_8);
                completed++;
            }
            println(
                "Decompiled " + completed + " mapped functions; failed " +
                failed + ".");
        }
        finally {
            decompiler.dispose();
        }
    }
}
