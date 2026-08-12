// Dump every analysis option for reproducible headless configuration.
// @category JediPowerBattles

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;

import ghidra.app.script.GhidraScript;

public class DumpAnalysisOptions extends GhidraScript {
    @Override
    public void run() throws Exception {
        Map<String, String> current =
            getCurrentAnalysisOptionsAndValues(currentProgram);
        Map<String, String> defaults = getAnalysisOptionDefaultValues(
            currentProgram, new ArrayList<String>(current.keySet()));
        List<String> names = new ArrayList<String>(current.keySet());
        Collections.sort(names);
        for (String name : names) {
            println(name + "\tcurrent=" + current.get(name) +
                "\tdefault=" + defaults.get(name));
        }
    }
}

