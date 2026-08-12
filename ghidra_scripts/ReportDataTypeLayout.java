// Reports an imported PDB data type without modifying the program.
// Usage: ReportDataTypeLayout.java <type-name> [...]

import ghidra.app.script.GhidraScript;
import ghidra.program.model.data.Composite;
import ghidra.program.model.data.DataType;
import ghidra.program.model.data.DataTypeComponent;
import ghidra.program.model.data.DataTypeManager;

import java.util.ArrayList;
import java.util.List;

public class ReportDataTypeLayout extends GhidraScript {
    @Override
    protected void run() throws Exception {
        String[] arguments = getScriptArgs();
        DataTypeManager manager = currentProgram.getDataTypeManager();

        if (arguments.length == 0) {
            printerr(
                "Usage: ReportDataTypeLayout.java <type-name> [...]");
            return;
        }

        for (String name : arguments) {
            List<DataType> matches = new ArrayList<>();

            manager.findDataTypes(name, matches);
            if (matches.isEmpty()) {
                println("TYPE " + name + " not found");
                continue;
            }
            for (DataType dataType : matches) {
                println(
                    "TYPE " + dataType.getPathName() +
                    " length=" + dataType.getLength() +
                    " display=\"" + dataType.getDisplayName() + "\"");
                if (dataType instanceof Composite) {
                    Composite composite = (Composite)dataType;

                    for (DataTypeComponent component :
                         composite.getDefinedComponents()) {
                        println(
                            "  offset=" + component.getOffset() +
                            " length=" + component.getLength() +
                            " field=\"" + component.getFieldName() + "\"" +
                            " type=\"" +
                            component.getDataType().getDisplayName() + "\"");
                    }
                }
            }
        }
    }
}
