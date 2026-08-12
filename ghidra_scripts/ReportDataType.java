// Reports the applied data type and direct composite members at an address.
// Usage: ReportDataType.java 0x140547B50

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.data.Composite;
import ghidra.program.model.data.DataType;
import ghidra.program.model.data.DataTypeComponent;
import ghidra.program.model.listing.Data;
import java.util.ArrayList;
import java.util.List;

public class ReportDataType extends GhidraScript {
    @Override
    public void run() throws Exception {
        if (getScriptArgs().length < 1 || getScriptArgs().length > 2) {
            printerr("usage: ReportDataType.java <address> [type-name]");
            return;
        }
        Address address = toAddr(getScriptArgs()[0]);
        Data data = getDataAt(address);
        if (data == null) {
            data = getDataContaining(address);
        }
        if (data == null) {
            println("ADDRESS " + address + " data=<none>");
            return;
        }
        DataType type = data.getDataType();
        println(
            "ADDRESS " + address + " type=" + type.getDisplayName() +
            " length=" + type.getLength());
        if (type instanceof Composite) {
            for (DataTypeComponent component :
                    ((Composite)type).getDefinedComponents()) {
                println(
                    "MEMBER offset=" + component.getOffset() +
                    " length=" + component.getLength() +
                    " name=" + component.getFieldName() +
                    " type=" +
                    component.getDataType().getDisplayName());
            }
        }
        if (getScriptArgs().length == 2) {
            List<DataType> matches = new ArrayList<DataType>();
            currentProgram.getDataTypeManager().findDataTypes(
                getScriptArgs()[1], matches);
            for (DataType match : matches) {
                println(
                    "MATCH path=" + match.getPathName() +
                    " length=" + match.getLength());
                if (match instanceof Composite) {
                    for (DataTypeComponent component :
                            ((Composite)match).getDefinedComponents()) {
                        println(
                            "MEMBER offset=" + component.getOffset() +
                            " length=" + component.getLength() +
                            " name=" + component.getFieldName() +
                            " type=" +
                            component.getDataType().getDisplayName());
                    }
                }
            }
        }
    }
}
