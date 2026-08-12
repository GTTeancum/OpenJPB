// Decompile one exact function entry to a UTF-8 text file.
// @category JediPowerBattles

import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;

public class DecompileAddress extends GhidraScript {
    private static final int DECOMPILE_TIMEOUT_SECONDS = 180;

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 2) {
            printerr(
                "Usage: DecompileAddress.java <address> <output-file>");
            return;
        }

        Address address = toAddr(Long.decode(args[0]));
        Path output = Paths.get(args[1]).toAbsolutePath().normalize();
        Function function = getFunctionAt(address);
        if (function == null) {
            throw new IllegalArgumentException(
                "No function starts at " + address);
        }

        DecompInterface decompiler = new DecompInterface();
        decompiler.toggleCCode(true);
        decompiler.toggleSyntaxTree(true);
        try {
            if (!decompiler.openProgram(currentProgram)) {
                throw new IllegalStateException(
                    "Could not initialize decompiler: " +
                    decompiler.getLastMessage());
            }
            DecompileResults result = decompiler.decompileFunction(
                function, DECOMPILE_TIMEOUT_SECONDS, monitor);
            if (!result.decompileCompleted() ||
                    result.getDecompiledFunction() == null) {
                throw new IllegalStateException(
                    "Decompilation failed: " + result.getErrorMessage());
            }
            if (output.getParent() != null) {
                Files.createDirectories(output.getParent());
            }
            Files.writeString(
                output,
                result.getDecompiledFunction().getC(),
                StandardCharsets.UTF_8);
            println("Decompiled " + function.getName() + " to " + output);
        }
        finally {
            decompiler.dispose();
        }
    }
}
