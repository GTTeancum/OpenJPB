// Report initialized MDEF_MOD records and their pointed-to globals.
// @category JediPowerBattles

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.mem.Memory;
import ghidra.program.model.symbol.Symbol;

public class ReportMdefMods extends GhidraScript {
    private static final int RECORD_SIZE = 32;

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 2) {
            printerr("Usage: ReportMdefMods.java <address> <count>");
            return;
        }
        Address array = toAddr(Long.decode(args[0]));
        int count = Integer.decode(args[1]);
        Memory memory = currentProgram.getMemory();

        for (int index = 0; index < count; ++index) {
            Address record = array.add((long)index * RECORD_SIZE);
            int type = Short.toUnsignedInt(memory.getShort(record));
            int increment =
                Short.toUnsignedInt(memory.getShort(record.add(2)));
            int minimum = memory.getInt(record.add(4));
            int maximum = memory.getInt(record.add(8));
            long pointer = memory.getLong(record.add(16));
            int text =
                Short.toUnsignedInt(memory.getShort(record.add(24)));
            Address target = toAddr(pointer);
            Symbol symbol =
                currentProgram.getSymbolTable().getPrimarySymbol(target);

            println(String.format(
                "MDEF_MOD index=%d address=%s type=0x%04X inc=%d " +
                "min=%d max=%d target=%s symbol=%s text=%d",
                index,
                record,
                type,
                increment,
                minimum,
                maximum,
                target,
                symbol == null ? "<none>" : symbol.getName(true),
                text));
        }
    }
}
