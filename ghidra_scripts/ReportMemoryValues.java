// Report raw 32-bit values at requested addresses.
// @category JediPowerBattles

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;

public class ReportMemoryValues extends GhidraScript {
    @Override
    public void run() throws Exception {
        for (String argument : getScriptArgs()) {
            Address address = toAddr(Long.decode(argument));
            int bits = getInt(address);
            println(address + "  0x" + String.format("%08X", bits) +
                "  int=" + bits + "  float=" + Float.intBitsToFloat(bits));
        }
    }
}
