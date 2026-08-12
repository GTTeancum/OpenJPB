// Dump direct code/data references to one or more addresses supplied as script
// arguments. Intended for repeatable PDB/decompiler provenance checks.
// @category JediPowerBattles

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionManager;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;
import ghidra.program.model.symbol.ReferenceManager;

public class DumpAddressReferences extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] arguments = getScriptArgs();
        FunctionManager functions = currentProgram.getFunctionManager();
        ReferenceManager references = currentProgram.getReferenceManager();

        if (arguments.length == 0) {
            printerr(
                "usage: DumpAddressReferences.java <address> [address ...]");
            return;
        }
        for (String text : arguments) {
            Address address = toAddr(text);
            ReferenceIterator iterator = references.getReferencesTo(address);
            int count = 0;

            println("REFERENCES " + address);
            while (iterator.hasNext()) {
                Reference reference = iterator.next();
                Address source = reference.getFromAddress();
                Function function = functions.getFunctionContaining(source);
                String name = function != null
                    ? function.getName()
                    : "<no-function>";

                println(
                    "  " + source + " " + reference.getReferenceType() +
                    " " + name);
                ++count;
            }
            println("  count=" + count);
        }
    }
}
