// Decompile every exact function entry in an address range.
// @category JediPowerBattles

import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;

public class DecompileRange extends GhidraScript {
    private static final int DECOMPILE_TIMEOUT_SECONDS = 180;

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 3) {
            printerr(
                "Usage: DecompileRange.java <start> <end> <output-dir>");
            return;
        }

        Address start = toAddr(Long.decode(args[0]));
        Address end = toAddr(Long.decode(args[1]));
        Path outputDirectory =
            Paths.get(args[2]).toAbsolutePath().normalize();
        Files.createDirectories(outputDirectory);

        DecompInterface decompiler = new DecompInterface();
        decompiler.toggleCCode(true);
        decompiler.toggleSyntaxTree(true);
        try {
            if (!decompiler.openProgram(currentProgram)) {
                throw new IllegalStateException(
                    "Could not initialize decompiler: " +
                    decompiler.getLastMessage());
            }

            FunctionIterator functions = currentProgram
                .getFunctionManager()
                .getFunctions(new AddressSet(start, end), true);
            int completed = 0;
            int failed = 0;
            while (functions.hasNext() && !monitor.isCancelled()) {
                Function function = functions.next();
                Address entry = function.getEntryPoint();
                if (entry.compareTo(start) < 0 || entry.compareTo(end) > 0) {
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
                    function.getName().replaceAll("[^A-Za-z0-9_.-]", "_") +
                    ".c";
                Files.writeString(
                    outputDirectory.resolve(fileName),
                    result.getDecompiledFunction().getC(),
                    StandardCharsets.UTF_8);
                completed++;
            }
            println(
                "Decompiled " + completed + " functions; failed " +
                failed + ".");
        }
        finally {
            decompiler.dispose();
        }
    }
}
