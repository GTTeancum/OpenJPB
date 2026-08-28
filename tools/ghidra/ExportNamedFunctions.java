// Export selected functions from an analyzed Ghidra program.
// @category JPB

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;

import java.io.File;
import java.io.PrintWriter;
import java.util.HashMap;
import java.util.Map;

public class ExportNamedFunctions extends GhidraScript {
    @Override
    protected void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 2) {
            throw new IllegalArgumentException(
                "usage: ExportNamedFunctions <output-directory> <name>...");
        }

        File outputDirectory = new File(args[0]);
        if (!outputDirectory.isDirectory() && !outputDirectory.mkdirs()) {
            throw new IllegalStateException(
                "could not create output directory: " + outputDirectory);
        }

        Map<String, Function> functions = new HashMap<>();
        FunctionIterator iterator =
            currentProgram.getFunctionManager().getFunctions(true);
        while (iterator.hasNext()) {
            Function function = iterator.next();
            functions.putIfAbsent(function.getName(), function);
        }

        DecompInterface decompiler = new DecompInterface();
        decompiler.toggleCCode(true);
        decompiler.toggleSyntaxTree(true);
        if (!decompiler.openProgram(currentProgram)) {
            throw new IllegalStateException("could not open program in decompiler");
        }

        try {
            for (int index = 1; index < args.length; ++index) {
                String specification = args[index];
                String name = specification;
                Function function = functions.get(name);
                int separator = specification.lastIndexOf('@');
                if (function == null && separator > 0) {
                    name = specification.substring(0, separator);
                    Address address = toAddr(
                        specification.substring(separator + 1));
                    function = currentProgram.getFunctionManager()
                        .getFunctionAt(address);
                }
                if (function == null) {
                    printerr("function not found: " + specification);
                    continue;
                }

                DecompileResults result =
                    decompiler.decompileFunction(function, 120, monitor);
                if (!result.decompileCompleted()) {
                    printerr("decompile failed for " + name + ": " +
                        result.getErrorMessage());
                    continue;
                }

                String address = function.getEntryPoint().toString();
                File output = new File(
                    outputDirectory, address + "_" + name + ".c");
                try (PrintWriter writer = new PrintWriter(output, "UTF-8")) {
                    writer.println("/* " + function.getName(true) + " @ " +
                        address + " */");
                    writer.print(
                        result.getDecompiledFunction().getC());
                }
                println("exported " + name + " -> " + output);
            }
        } finally {
            decompiler.dispose();
        }
    }
}
