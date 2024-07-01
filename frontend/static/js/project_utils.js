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