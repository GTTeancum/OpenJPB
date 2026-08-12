// Report one or more pointer arrays as <address>:<count>.
// @category JediPowerBattles

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.mem.Memory;

public class ReportPointerStrings extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length == 0) {
            printerr("Usage: ReportPointerStrings.java <address>:<count> [...]");
            return;
        }

        Memory memory = currentProgram.getMemory();
        int pointerSize = currentProgram.getDefaultPointerSize();
        for (String argument : args) {
            int separator = argument.lastIndexOf(':');
            if (separator <= 0 || separator == argument.length() - 1) {
                printerr("Expected <address>:<count>, got " + argument);
                continue;
            }

            Address array =
                toAddr(Long.decode(argument.substring(0, separator)));
            int count = Integer.parseInt(argument.substring(separator + 1));
            for (int index = 0; index < count; ++index) {
                Address slot = array.add((long)index * pointerSize);
                long value = pointerSize == 8
                    ? memory.getLong(slot)
                    : Integer.toUnsignedLong(memory.getInt(slot));
                Address target = toAddr(value);
                String stringValue;
                try {
                    StringBuilder text = new StringBuilder();
                    for (int offset = 0; offset < 1024; ++offset) {
                        byte next = memory.getByte(target.add(offset));
                        if (next == 0) {
                            break;
                        }
                        text.append((char)Byte.toUnsignedInt(next));
                    }
                    stringValue = text.toString();
                }
                catch (Exception error) {
                    stringValue = "<unreadable>";
                }
                println("POINTER_STRING array=" + array +
                    " index=" + index +
                    " target=" + target +
                    " value=\"" + stringValue.replace("\"", "\\\"") + "\"");
            }
        }
    }
}
