// Report one or more float arrays as <address>:<count>.
// @category JediPowerBattles

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.mem.Memory;

public class ReportFloatArrays extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length == 0) {
            printerr("Usage: ReportFloatArrays.java <address>:<count> [...]");
            return;
        }

        Memory memory = currentProgram.getMemory();
        for (String argument : args) {
            int separator = argument.lastIndexOf(':');
            if (separator <= 0 || separator == argument.length() - 1) {
                printerr("Expected <address>:<count>, got " + argument);
                continue;
            }

            Address address =
                toAddr(Long.decode(argument.substring(0, separator)));
            int count = Integer.parseInt(argument.substring(separator + 1));
            StringBuilder values = new StringBuilder();
            byte[] bytes = new byte[4];
            for (int index = 0; index < count; ++index) {
                memory.getBytes(address.add((long)index * 4), bytes);
                int bits =
                    Byte.toUnsignedInt(bytes[0]) |
                    (Byte.toUnsignedInt(bytes[1]) << 8) |
                    (Byte.toUnsignedInt(bytes[2]) << 16) |
                    (Byte.toUnsignedInt(bytes[3]) << 24);
                if (index != 0) {
                    values.append(", ");
                }
                values.append(Float.intBitsToFloat(bits));
            }
            println("FLOATS " + address + " count=" + count +
                " values=[" + values + "]");
        }
    }
}
