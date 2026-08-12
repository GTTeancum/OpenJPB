// Report all static references to one or more addresses.
// @category JediPowerBattles

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.CodeUnit;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;
import ghidra.program.model.symbol.Symbol;

public class ReportAddressReferences extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length == 0) {
            printerr("Usage: ReportAddressReferences.java <address> [...]");
            return;
        }

        for (String argument : args) {
            Address target = toAddr(Long.decode(argument));
            Symbol symbol = currentProgram.getSymbolTable().getPrimarySymbol(target);
            println("TARGET " + target +
                (symbol == null ? "" : " " + symbol.getName(true)));

            ReferenceIterator references =
                currentProgram.getReferenceManager().getReferencesTo(target);
            int count = 0;
            while (references.hasNext() && !monitor.isCancelled()) {
                Reference reference = references.next();
                Address from = reference.getFromAddress();
                Function function =
                    currentProgram.getFunctionManager().getFunctionContaining(from);
                CodeUnit unit = currentProgram.getListing().getCodeUnitAt(from);
                println("  " + from +
                    " " + reference.getReferenceType() +
                    (function == null ? "" :
                        " function=" + function.getName() +
                        " entry=" + function.getEntryPoint()) +
                    (unit == null ? "" : " instruction=\"" +
                        unit.toString().replace("\"", "\\\"") + "\""));
                ++count;
            }
            println("REFERENCES " + count);
        }
    }
}
