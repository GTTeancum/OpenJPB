// Disable pathological whole-PDB type application and seed exact PDB bodies.
// @category JediPowerBattles

import java.io.BufferedReader;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionManager;
import ghidra.program.model.symbol.SourceType;

public class ConfigureAndSeedFunctions extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 1) {
            printerr("Usage: ConfigureAndSeedFunctions.java <function-map.tsv>");
            return;
        }

        setAnalysisOption(currentProgram, "PDB Universal", "false");

        Path functionMap = Paths.get(args[0]).toAbsolutePath().normalize();
        FunctionManager manager = currentProgram.getFunctionManager();
        int created = 0;
        int existing = 0;
        int failed = 0;

        try (BufferedReader reader = Files.newBufferedReader(
                functionMap, StandardCharsets.UTF_8)) {
            String line = reader.readLine(); // header
            while ((line = reader.readLine()) != null && !monitor.isCancelled()) {
                String[] fields = line.split("\\t", -1);
                if (fields.length < 6) {
                    failed++;
                    continue;
                }
                Address entry = toAddr(Long.decode(fields[0]));
                long size = Long.parseLong(fields[2]);
                Function function = manager.getFunctionAt(entry);
                if (function != null) {
                    existing++;
                    continue;
                }
                if (size <= 0) {
                    failed++;
                    continue;
                }
                Address end = entry.add(size - 1);
                AddressSet body = new AddressSet(entry, end);
                String name = "pdb_" + fields[1].substring(2);
                try {
                    manager.createFunction(
                        name, entry, body, SourceType.IMPORTED);
                    created++;
                }
                catch (Exception error) {
                    printerr("Could not seed " + fields[5] + " at " +
                        fields[0] + ": " + error.getMessage());
                    failed++;
                }
            }
        }

        println("PDB Universal disabled; seeded " + created +
            " exact functions, retained " + existing +
            " existing functions, failed " + failed + ".");
    }
}

