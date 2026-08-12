// Dump exact bytes from one or more virtual-address ranges.
// @category JediPowerBattles

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.mem.Memory;

public class DumpMemoryRanges extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();

        if (args.length == 0 || (args.length & 1) != 0) {
            printerr("Usage: DumpMemoryRanges.java <address> <byte-count> ...");
            return;
        }
        Memory memory = currentProgram.getMemory();
        for (int range = 0; range < args.length; range += 2) {
            Address start = toAddr(Long.decode(args[range]));
            int byteCount = Integer.decode(args[range + 1]);
            byte[] bytes = new byte[byteCount];

            memory.getBytes(start, bytes);
            for (int offset = 0; offset < byteCount; offset += 16) {
                StringBuilder line = new StringBuilder();
                line.append(start.add(offset)).append(":");
                int lineEnd = Math.min(offset + 16, byteCount);

                for (int index = offset; index < lineEnd; ++index) {
                    line.append(String.format(" %02x", bytes[index] & 0xff));
                }
                println(line.toString());
            }
        }
    }
}
