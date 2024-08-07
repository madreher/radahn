var radahnProjectUtils = {};

radahnProjectUtils.viewerToXYZ = function(viewer, frameIndex)
{
    if(frameIndex < 0 || frameIndex >= viewer.getModel().getNumFrames())
    {
        console.error("Frame index requested out of bounds: " + frameIndex);
        return "";
    }
    let frame = viewer.getModel().frames[frameIndex];

    if(frame.length == 0)
    {
        console.error("Frame is empty: " + frameIndex);
        return "";
    }

    let xyz = "";
    xyz += frame.length + "\n";
    xyz += "Frame: " + frameIndex + "\n";
    for(let i = 0; i < frame.length; i++)
    {
        xyz += frame[i].elem + " " + frame[i].x + " " + frame[i].y + " " + frame[i].z + "\n";
    }
    return xyz;
}

radahnProjectUtils.viewerToArray = function(viewer, frameIndex)
{
    if(frameIndex < 0 || frameIndex >= viewer.getModel().getNumFrames())
    {
        console.error("Frame index requested out of bounds: " + frameIndex);
        return "";
    }
    let frame = viewer.getModel().frames[frameIndex];

    if(frame.length == 0)
    {
        console.error("Frame is empty: " + frameIndex);
        return "";
    }

    let positions = []
    for(let i = 0; i < frame.length; i++)
    {
        positions.push([frame[i].x, frame[i].y, frame[i].z]);
    }

    return positions;

}

class RadahnProject {

    // We keep everything public for now for simplicity. This is basically just a data storage
    projectName = "defaultProjectName";
    xyzContent = "";
    xyzFilename = "";
    potentialContent = "";
    potentialFilename = "";
    motorGraph = "";
    motorGraphUnits = "LAMMPS_REAL";
    anchorList = {};
    selectionList = {};
    thermostatList = {};
    minimizeEnabled = false;
    minimizeConfig = {};
    nvtEnabled = false;
    nvtConfig = {};

    constructor()
    {
        
    }

    resetProject()
    {
        this.projectName = "defaultProjectName";
        this.xyzContent = "";
        this.xyzFilename = "";
        this.potentialContent = "";
        this.potentialFilename = "";
        this.potentialType = "";
        this.motorGraph = {};
        this.motorGraphUnits = "LAMMPS_REAL";
        this.anchorList = {};
        this.selectionList = {};
        this.thermostatList = {};
        this.minimizeEnabled = false;
        this.minimizeConfig = {};
        this.nvtEnabled = false;
        this.nvtConfig = {};
    }

    setProjectName(projectName)
    {
        this.projectName = projectName;
    }

    setXYZContent(xyzContent, xyzFilename)
    {
        this.xyzContent = xyzContent;
        this.xyzFilename = xyzFilename;
    }

    isXYZSet()
    {
        return this.xyzContent.length > 0;
    }

    setPotential(potentialContent, potentialFilename)
    {
        this.potentialContent = potentialContent;
        this.potentialFilename = potentialFilename;
        this.potentialType = potentialFilename.split('.').pop();
    }

    setMotorGraph(motorGraph, motorGraphUnits)
    {
        this.motorGraph = motorGraph;
        this.motorGraphUnits = motorGraphUnits;
    }

    hasMotorGraph()
    {
        return this.motorGraph.length > 0;
    }

    addAnchor(anchorName, anchor)
    {
        this.anchorList[anchorName] = anchor;
    }

    deleteAnchor(anchorName)
    {
        delete this.anchorList[anchorName];
    }

    addSelection(selectionName, selection)
    {
        this.selectionList[selectionName] = selection;
    }

    deleteSelection(selectionName)
    {
        delete this.selectionList[selectionName];
    }

    addThermostat(thermostatName, thermostat)
    {
        this.thermostatList[thermostatName] = thermostat;
    }

    deleteThermostat(thermostatName)
    {
        delete this.thermostatList[thermostatName];
    }

    setSelectionPositionsFromXYZ(xyzContent)
    {
        // Parse the xyz content
        const lines = xyzContent.split('\n');

        // The first line is the number of atoms
        const atomCount = parseInt(lines[0].trim(), 10);

        // The second line is the comment line, usually ignored or used as metadata
        const comment = lines[1].trim();

        // The remaining lines are the atomic coordinates
        const atoms = [];
        for (let i = 2; i < lines.length; i++) {
            const line = lines[i].trim();
            if (line) {
                const [element, x, y, z] = line.split(/\s+/);
                atoms.push({
                    element,
                    x: parseFloat(x),
                    y: parseFloat(y),
                    z: parseFloat(z)
                });
            }
        }

        // Now that we have the list of atoms and their positions, we can update each selection list
        for ( let [name, selection] of Object.entries(this.selectionList)) {
            let indices = selection.atomIndexes;
            let avgx = 0;
            let avgy = 0; 
            let avgz = 0;
            indices.forEach((val) => {
                avgx += atoms[val].x;
                avgy += atoms[val].y;
                avgz += atoms[val].z;
            });

            avgx /= indices.length;
            avgy /= indices.length;
            avgz /= indices.length;

            selection.centroid = [avgx, avgy, avgz];
        };
    }

    declareMinimize()
    {
        this.minimizeConfig = {
            "type": "regular"
        };
        this.minimizeEnabled = true;
    }

    declareDeepMinimize()
    {
        this.minimizeConfig = {
            "type": "deep"
        };
        this.minimizeEnabled = true;
    }

    clearMinimize()
    {
        this.minimizeConfig = {}
        this.minimizeEnabled = false;
    }

    declareNVTCreateVelocity(temp, seed)
    {
        this.nvtConfig = {
            "type": "createVelocity",
            "temp": temp, 
            "seed": seed
        };
        this.nvtEnabled = true;
    }

    declareNVTPhase(startTemp, endTemp, dampingFactor, seed, nbSteps)
    {
        this.nvtConfig = {
            "type": "nvtPhase",
            "startTemp": startTemp,
            "endTemp": endTemp,
            "damp": dampingFactor,
            "seed": seed,
            "steps": nbSteps
        };
        this.nvtEnabled = true;
    }

    clearNVT()
    {
        this.nvtConfig = {};
        this.nvtEnabled = false;
    }

    createProjectDict()
    {
        let project = {}
        project["header"] = {
            "format": "radahn",
            "versionMajor": 0,
            "versionMinor": 1,
            "generator": "radahn_frontend_localhost"
        }
        project["projectName"] = this.projectName;
        project["xyzContent"] = this.xyzContent;
        project["xyzFilename"] = this.xyzFilename;
        project["potentialContent"] = this.potentialContent;
        project["potentialFilename"] = this.potentialFilename;
        project["motorGraph"] = this.motorGraph;
        project["motorGraphUnits"] = this.motorGraphUnits;
        project["anchors"] = this.anchorList;
        project["selections"] = this.selectionList;
        project["thermostats"] = this.thermostatList;
        if(this.minimizeEnabled)
        {
            project["minimizeConfig"] = this.minimizeConfig;
        }
        if(this.nvtEnabled)
        {
            project["nvtConfig"] = this.nvtConfig;
        }

        return project;
    }

    loadFromProjectDict(dictContent)
    {
        // Check for the header information 
        if (!("header" in dictContent))
        {
            console.error("Unable to find the header when loading a project file.");
            return;
        }

        if(!("format" in dictContent["header"]))
        {
            console.error("Unable to find the format in the header of the project file.");
            return;
        }

        if(dictContent["header"]["format"] != "radahn")
        {
            console.error("Incompatible format found in the project file.");
            return;
        }

        // Do something here once the versions start to diverge

        // Clear the project
        this.resetProject()

        // Load field by field S
        if("projectName" in dictContent)
            this.projectName = dictContent["projectName"];
        if("xyzContent" in dictContent)
            this.xyzContent = dictContent["xyzContent"];
        if("xyzFilename" in dictContent)
            this.xyzFilename = dictContent["xyzFilename"];
        if("potentialContent" in dictContent)
            this.potentialContent = dictContent["potentialContent"];
        if("potentialFilename" in dictContent)
            this.potentialFilename = dictContent["potentialFilename"];
            this.potentialType = this.potentialFilename.split('.').pop();
        if("anchors" in dictContent)
            this.anchorList = dictContent["anchors"];
        if("selections" in dictContent)
            this.selectionList = dictContent["selections"];
        if("thermostats" in dictContent)
            this.thermostatList = dictContent["thermostats"];
        if("motorGraph" in dictContent)
            this.motorGraph = dictContent["motorGraph"];
        if("motorGraphUnits" in dictContent)
            this.motorGraphUnits = dictContent["motorGraphUnits"];
        if("minimizeConfig" in dictContent)
        {
            this.minimizeEnabled = true;
            this.minimizeConfig = dictContent["minimizeConfig"];
        }
        if("nvtConfig" in dictContent)
        {
            this.nvtEnabled = true;
            this.nvtConfig = dictContent["nvtConfig"];
        }

    }

    generateLammpsConfigJSON = function(unit)
    {
        let anchorNodes = [];
        // Go though each element of the anchor table
        for(const [key, value] of Object.entries(this.anchorList))
        {
            let anchorNode = {
                "name": key,
                "selection": value.atomIndexes.slice()
            };
            console.log(anchorNode);
            for(let j = 0; j < anchorNode["selection"].length; ++j)
            {
                anchorNode["selection"][j] = anchorNode["selection"][j] + 1;
            }
            anchorNodes.push(anchorNode);
        }

        let thermostatNodes = [];
        for(const [key, value] of Object.entries(this.thermostatList))
        {
            let thermostatNode = {
                "name": key,
                "type": "langevin",
                "selection": value.atomIndexes.slice(),
                "startTemp": value.startTemp,
                "endTemp": value.endTemp,
                "seed" : value.seed,
                "damp": value.damp
            };
            console.log(thermostatNode);
            for(let j = 0; j < thermostatNode["selection"].length; ++j)
            {
                thermostatNode["selection"][j] = thermostatNode["selection"][j] + 1;
            }
            thermostatNodes.push(thermostatNode);
        }

        let radahnJSON = {}
        radahnJSON["header"] = { "versionMajor": 0, "versionMinor": 1, "units": unit, "generator": "Radahn_frontendV1", "format": "lmpConfig", "fftype": this.potentialType};
        radahnJSON["anchors"] = anchorNodes;
        radahnJSON["thermostats"] = thermostatNodes;
        if(this.nvtEnabled)
        {
            radahnJSON["nvtConfig"] = this.nvtConfig;
        }

        return JSON.stringify(radahnJSON, null, 4);
    }
}