// Export Ghidra decompiler output according to inventory/function_map.tsv.
// @category JediPowerBattles

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardOpenOption;
import java.util.HashMap;
import java.util.Map;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;

public class ExportReconstruction extends GhidraScript {
    private static final int DECOMPILE_TIMEOUT_SECONDS = 120;

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 2) {
            printerr("Usage: ExportReconstruction.java <output-dir> <function-map.tsv>");
            return;
        }

        Path outputRoot = Paths.get(args[0]).toAbsolutePath().normalize();
        Path functionMap = Paths.get(args[1]).toAbsolutePath().normalize();
        Files.createDirectories(outputRoot);

        DecompInterface decompiler = new DecompInterface();
        decompiler.toggleCCode(true);
        decompiler.toggleSyntaxTree(true);
        if (!decompiler.openProgram(currentProgram)) {
            throw new IOException("Could not initialize decompiler: " +
                decompiler.getLastMessage());
        }

        Map<Path, BufferedWriter> writers = new HashMap<>();
        int exported = 0;
        int missing = 0;
        int failed = 0;

        try (BufferedReader reader = Files.newBufferedReader(
                functionMap, StandardCharsets.UTF_8)) {
            String line = reader.readLine(); // header
            if (line == null) {
                throw new IOException("Function map is empty");
            }
            while ((line = reader.readLine()) != null && !monitor.isCancelled()) {
                String[] fields = line.split("\\t", -1);
                if (fields.length < 6) {
                    printerr("Skipping malformed function-map row: " + line);
                    failed++;
                    continue;
                }

                long addressValue = Long.decode(fields[0]);
                Address address = toAddr(addressValue);
                Function function = getFunctionAt(address);
                if (function == null) {
                    printerr("No exact Ghidra function entry at " + fields[0] +
                        " for " + fields[5]);
                    missing++;
                    continue;
                }

                Path relative = Paths.get(fields[4]).normalize();
                Path output = outputRoot.resolve(relative).normalize();
                if (!output.startsWith(outputRoot)) {
                    printerr("Refusing output path outside root: " + fields[4]);
                    failed++;
                    continue;
                }
                Files.createDirectories(output.getParent());
                BufferedWriter writer = writers.get(output);
                if (writer == null) {
                    writer = Files.newBufferedWriter(
                        output,
                        StandardCharsets.UTF_8,
                        StandardOpenOption.CREATE,
                        StandardOpenOption.TRUNCATE_EXISTING,
                        StandardOpenOption.WRITE);
                    writer.write("/* Raw Ghidra decompiler export. Review before integration. */\n\n");
                    writers.put(output, writer);
                }

                DecompileResults result = decompiler.decompileFunction(
                    function, DECOMPILE_TIMEOUT_SECONDS, monitor);
                writer.write("/* PDB: " + fields[5] + " | " + fields[1] +
                    " | " + fields[2] + " bytes | Ghidra: " +
                    function.getName() + " */\n");
                if (!result.decompileCompleted() ||
                        result.getDecompiledFunction() == null) {
                    String error = result.getErrorMessage();
                    if (error == null || error.isBlank()) {
                        error = "unknown decompiler error";
                    }
                    writer.write("/* DECOMPILATION FAILED: " +
                        error.replace("*/", "* /") +
                        " */\n\n");
                    failed++;
                    continue;
                }
                writer.write(result.getDecompiledFunction().getC());
                writer.write("\n\n");
                exported++;
                if ((exported % 100) == 0) {
                    println("Exported " + exported + " functions...");
                }
            }
        } finally {
            for (BufferedWriter writer : writers.values()) {
                writer.close();
            }
            decompiler.dispose();
        }

        Path summary = outputRoot.resolve("EXPORT_SUMMARY.txt");
        Files.writeString(
            summary,
            "Program: " + currentProgram.getName() + "\n" +
            "Image base: " + currentProgram.getImageBase() + "\n" +
            "Exported: " + exported + "\n" +
            "Missing functions: " + missing + "\n" +
            "Decompiler failures: " + failed + "\n",
            StandardCharsets.UTF_8,
            StandardOpenOption.CREATE,
            StandardOpenOption.TRUNCATE_EXISTING);
        println("Export complete: " + summary);
    }
}
