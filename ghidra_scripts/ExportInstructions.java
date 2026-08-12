// Export an exact instruction range to a UTF-8 text file.
// @category JediPowerBattles

import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;

public class ExportInstructions extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 3) {
            printerr(
                "Usage: ExportInstructions.java " +
                "<start-address> <end-address> <output-file>");
            return;
        }

        Address start = toAddr(Long.decode(args[0]));
        Address end = toAddr(Long.decode(args[1]));
        Path output = Paths.get(args[2]).toAbsolutePath().normalize();
        StringBuilder text = new StringBuilder();
        InstructionIterator instructions = currentProgram
            .getListing()
            .getInstructions(start, true);

        while (instructions.hasNext()) {
            Instruction instruction = instructions.next();
            if (instruction.getAddress().compareTo(end) >= 0) {
                break;
            }
            text.append(instruction.getAddress())
                .append("  ")
                .append(instruction)
                .append(System.lineSeparator());
        }
        if (output.getParent() != null) {
            Files.createDirectories(output.getParent());
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
        println("Exported instructions to " + output);
    }
}
