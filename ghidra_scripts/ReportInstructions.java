// Report decoded instructions in one or more half-open address ranges.
// @category JediPowerBattles

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;

public class ReportInstructions extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length == 0) {
            printerr(
                "Usage: ReportInstructions.java <start>:<end-exclusive> [...]");
            return;
        }

        for (String argument : args) {
            int separator = argument.lastIndexOf(':');
            if (separator <= 0 || separator == argument.length() - 1) {
                printerr("Expected <start>:<end-exclusive>, got " + argument);
                continue;
            }

            Address start =
                toAddr(Long.decode(argument.substring(0, separator)));
            Address endExclusive =
                toAddr(Long.decode(argument.substring(separator + 1)));
            AddressSet range =
                new AddressSet(start, endExclusive.subtract(1));
            InstructionIterator instructions =
                currentProgram.getListing().getInstructions(range, true);

            println("INSTRUCTIONS " + start + ".." + endExclusive);
            while (instructions.hasNext()) {
                Instruction instruction = instructions.next();
                println(instruction.getAddress() + "  " + instruction);
            }
        }
    }
}
