// Dump exact bytes from one virtual-address range.
// @category JediPowerBattles

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.mem.Memory;

public class DumpMemoryBytes extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();

        if (args.length != 2) {
            printerr("Usage: DumpMemoryBytes.java <address> <byte-count>");
            return;
        }
        Address start = toAddr(Long.decode(args[0]));
        int byteCount = Integer.decode(args[1]);
        byte[] bytes = new byte[byteCount];
        Memory memory = currentProgram.getMemory();

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
