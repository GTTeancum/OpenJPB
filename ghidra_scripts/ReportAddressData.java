// Report symbols and raw data at one or more addresses.
// @category JediPowerBattles

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.Symbol;

public class ReportAddressData extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length == 0) {
            printerr("Usage: ReportAddressData.java <address> [...]");
            return;
        }

        for (String argument : args) {
            Address address = toAddr(Long.decode(argument));
            Symbol symbol =
                currentProgram.getSymbolTable().getPrimarySymbol(address);
            MemoryBlock block = currentProgram.getMemory().getBlock(address);
            byte[] bytes = new byte[32];
            int read = currentProgram.getMemory().getBytes(address, bytes);
            int littleEndianWord =
                Byte.toUnsignedInt(bytes[0]) |
                (Byte.toUnsignedInt(bytes[1]) << 8) |
                (Byte.toUnsignedInt(bytes[2]) << 16) |
                (Byte.toUnsignedInt(bytes[3]) << 24);

            StringBuilder hex = new StringBuilder();
            for (int index = 0; index < read; ++index) {
                if (index != 0) {
                    hex.append(' ');
                }
                hex.append(String.format("%02x", Byte.toUnsignedInt(bytes[index])));
            }

            println("ADDRESS " + address +
                (symbol == null ? "" : " symbol=" + symbol.getName(true)) +
                (block == null ? "" : " block=" + block.getName()) +
                " u32=0x" + Integer.toUnsignedString(littleEndianWord, 16) +
                " i32=" + littleEndianWord +
                " f32=" + Float.intBitsToFloat(littleEndianWord) +
                " bytes=" + hex);
        }
    }
}
