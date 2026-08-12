// Dump little-endian float constants at exact virtual addresses.
// @category JediPowerBattles

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.mem.Memory;

public class DumpFloatAddresses extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        Memory memory = currentProgram.getMemory();

        if (args.length == 0) {
            printerr("Usage: DumpFloatAddresses.java <address>...");
            return;
        }
        for (String argument : args) {
            Address address = toAddr(Long.decode(argument));
            int bits = memory.getInt(address, false);
            println(
                argument + " bits=0x" +
                Integer.toUnsignedString(bits, 16) +
                " float=" + Float.intBitsToFloat(bits));
        }
    }
}
