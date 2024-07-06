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

    xyzContent = "";
    xyzFilename = "";
    potentialContent = "";
    potentialFilename = "";
    motorGraph = {};
    motorGraphUnits = "LAMMPS_REAL";
    anchorList = {}
    selectionList = {}

    constructor()
    {
        
    }

    setXYZContent(xyzContent, xyzFilename)
    {
        this.xyzContent = xyzContent;
        this.xyzFilename = xyzFilename;
    }

    setPotential(potentialContent, potentialFilename)
    {
        this.potentialContent = potentialContent;
        this.potentialFilename = potentialFilename;
    }

    setMotorGraph(motorGraph, motorGraphUnits)
    {
        this.motorGraph = motorGraph;
        this.motorGraphUnits = motorGraphUnits;
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

    createProjectDict()
    {
        let project = {}
        project["xyzContent"] = this.xyzContent;
        project["xyzFilename"] = this.xyzFilename;
        project["potentialContent"] = this.potentialContent;
        project["potentialFilename"] = this.potentialFilename;
        project["motorGraph"] = this.motorGraph;
        project["motorGraphUnits"] = this.motorGraphUnits;
        project["anchors"] = this.anchorList;
        project["selections"] = this.selectionList;

        return project;
    }
}