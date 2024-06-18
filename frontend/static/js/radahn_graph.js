
var radahnGraphUtil = {}

radahnGraphUtil.setupRadahnGraph = function(lGraph)
{
    console.log("calling setupRadahnGraph");

    // Remove the default types of nodes, we will only used types defined in this utility module.
    //console.log(lGraph);
    LiteGraph.clearRegisteredTypes();

    // Blank motor
    function BlankNode()
    {
        this.addInput("Dependency", "string");
        this.addOutput("MotorName", "string");
        this.properties = {
            name: "defaultBlank", 
            dependencies: [],
            nbSteps: 1000, 
            valid: true, 
            errorMsg: "OK"
        };
        this.widgetName = this.addWidget("text", "Name", this.properties.name, {property: "name"});
        this.widgetNbSteps = this.addWidget("number", "NbSteps", this.properties.nbSteps, {property: "nbSteps", min: 1, max: 10000000000, step:1});
        //this.widgets_up = true;
        this.size = [300, 80];
    }
    BlankNode.title = "Blank Motor"
    BlankNode.prototype.onExecute = function(){
        this.properties.dependencies = [this.getInputData(0)];
        if(this.properties.nbSteps <= 0)
        {
            this.properties.valid = false;
            this.properties.error = "The number of steps must be grater than 0.";
        }
        if(this.properties.name.length <= 0)
        {
            this.properties.valid = false;
            this.properties.error = "The name of a motor cannot be empty.";
        }

        this.setOutputData(0, this.properties.name);
    }

    BlankNode.prototype.onGetRadahnDict = function(motorArray)
    {
        // Has the watch the format needed by blankMotor.cpp
        result = {
            "type": "blank",
            "dependencies": this.properties.dependencies,
            "name": this.properties.name,
            "nbStepsRequested": this.properties.nbSteps
        };

        motorArray.push(result);
    }

    LiteGraph.registerNodeType("motor/BlankMotor", BlankNode);

    // Move motor
    function MoveMotor()
    {
        this.addInput("Dependency", "string");
        this.addOutput("MotorName", "string");
        this.properties = {
            name: "defaultMove", 
            dependencies: [],
            selectionName: "",
            vx: 0.0,
            vy: 0.0,
            vz: 0.0,
            checkx: false,
            checky: false,
            checkx: false,
            dx: 0.0, 
            dy: 0.0,
            dz: 0.0,
            valid: true, 
            errorMsg: "OK"
        };

        this.widgetName = this.addWidget("text", "Name", this.properties.name, {property: "name"});
        this.widgetSelection = this.addWidget("text", "Selection", this.properties.selectionName, {property: "selectionName"});
        this.widgetVX = this.addWidget("number", "vx (velocity unit)", this.properties.vx, {property: "vx", min: -10000000000.0, max: 10000000000.0, step:1});
        this.widgetVY = this.addWidget("number", "vy (velocity unit)", this.properties.vy, {property: "vy", min: -10000000000.0, max: 10000000000.0, step:1});
        this.widgetVZ = this.addWidget("number", "vz (velocity unit)", this.properties.vz, {property: "vz", min: -10000000000.0, max: 10000000000.0, step:1});
        this.widgetCheckX = this.addWidget("toggle", "checkX", this.properties.checkx, {property: "checkx"});
        this.widgetCheckY = this.addWidget("toggle", "checkY", this.properties.checky, {property: "checky"});
        this.widgetCheckZ = this.addWidget("toggle", "checkZ", this.properties.checkz, {property: "checkz"});
        this.widgetDX = this.addWidget("number", "dx (distance unit)", this.properties.dx, {property: "dx", min: -10000000000.0, max: 10000000000.0, step:1});
        this.widgetDY = this.addWidget("number", "dy (distance unit)", this.properties.dy, {property: "dy", min: -10000000000.0, max: 10000000000.0, step:1});
        this.widgetDZ = this.addWidget("number", "dz (distance unit)", this.properties.dz, {property: "dz", min: -10000000000.0, max: 10000000000.0, step:1});

        this.size = [400, 300];
    }

    MoveMotor.title = "Move Motor"
    MoveMotor.prototype.onExecute = function(){
        this.properties.dependencies = [this.getInputData(0)];
        if(this.properties.name.length <= 0)
        {
            this.properties.valid = false;
            this.properties.error = "The name of a motor cannot be empty.";
        }
        if(this.properties.selectionName.length <= 0)
        {
            this.properties.valid = false;
            this.properties.error = "The name of a selection cannot be empty.";
        }

        if(!this.properties.checkx && !this.properties.checky && !this.properties.checkz)
        {
            this.properties.valid = false;
            this.properties.error = "At least one direction must be checked.";
        }

        this.setOutputData(0, this.properties.name);
    }
    MoveMotor.prototype.onGetRadahnDict = function(motorArray)
    {
        result = {
            "type": "move",
            "dependencies": this.properties.dependencies,
            "name": this.properties.name,
            "selectionName" : this.properties.selectionName,
            "vx": this.properties.vx,
            "vy": this.properties.vy,
            "vz": this.properties.vz,
            "checkX": this.properties.checkx,
            "checkY": this.properties.checky,
            "checkZ": this.properties.checkz,
            "dx": this.properties.dx,
            "dy": this.properties.dy,
            "dz": this.properties.dz
        };

        motorArray.push(result);
    }
    LiteGraph.registerNodeType("motor/MoveMotor", MoveMotor);

    // Force motor
    function ForceMotor()
    {
        this.addInput("Dependency", "string");
        this.addOutput("MotorName", "string");
        this.properties = {
            name: "defaultMove", 
            dependencies: [],
            selectionName: "",
            fx: 0.0,
            fy: 0.0,
            fz: 0.0,
            checkx: false,
            checky: false,
            checkx: false,
            dx: 0.0, 
            dy: 0.0,
            dz: 0.0,
            valid: true, 
            errorMsg: "OK"
        };

        this.widgetName = this.addWidget("text", "Name", this.properties.name, {property: "name"});
        this.widgetSelection = this.addWidget("text", "Selection", this.properties.selectionName, {property: "selectionName"});
        this.widgetFX = this.addWidget("number", "fx (force unit)", this.properties.fx, {property: "fx", min: -10000000000.0, max: 10000000000.0, step:1});
        this.widgetFY = this.addWidget("number", "fy (force unit)", this.properties.fy, {property: "fy", min: -10000000000.0, max: 10000000000.0, step:1});
        this.widgetFZ = this.addWidget("number", "fz (force unit)", this.properties.fz, {property: "fz", min: -10000000000.0, max: 10000000000.0, step:1});
        this.widgetCheckX = this.addWidget("toggle", "checkX", this.properties.checkx, {property: "checkx"});
        this.widgetCheckY = this.addWidget("toggle", "checkY", this.properties.checky, {property: "checky"});
        this.widgetCheckZ = this.addWidget("toggle", "checkZ", this.properties.checkz, {property: "checkz"});
        this.widgetDX = this.addWidget("number", "dx (distance unit)", this.properties.dx, {property: "dx", min: -10000000000.0, max: 10000000000.0, step:1});
        this.widgetDY = this.addWidget("number", "dy (distance unit)", this.properties.dy, {property: "dy", min: -10000000000.0, max: 10000000000.0, step:1});
        this.widgetDZ = this.addWidget("number", "dz (distance unit)", this.properties.dz, {property: "dz", min: -10000000000.0, max: 10000000000.0, step:1});

        this.size = [400, 300];
    }

    ForceMotor.title = "Force Motor"
    ForceMotor.prototype.onExecute = function(){
        this.properties.dependencies = [this.getInputData(0)];
        if(this.properties.name.length <= 0)
        {
            this.properties.valid = false;
            this.properties.error = "The name of a motor cannot be empty.";
        }
        if(this.properties.selectionName.length <= 0)
        {
            this.properties.valid = false;
            this.properties.error = "The name of a selection cannot be empty.";
        }

        if(!this.properties.checkx && !this.properties.checky && !this.properties.checkz)
        {
            this.properties.valid = false;
            this.properties.error = "At least one direction must be checked.";
        }

        this.setOutputData(0, this.properties.name);
    }

    ForceMotor.prototype.onGetRadahnDict = function(motorArray)
    {
        result = {
            "type": "force",
            "dependencies": this.properties.dependencies,
            "name": this.properties.name,
            "selectionName" : this.properties.selectionName,
            "fx": this.properties.fx,
            "fy": this.properties.fy,
            "fz": this.properties.fz,
            "checkX": this.properties.checkx,
            "checkY": this.properties.checky,
            "checkZ": this.properties.checkz,
            "dx": this.properties.dx,
            "dy": this.properties.dy,
            "dz": this.properties.dz
        };

        motorArray.push(result);
    }

    LiteGraph.registerNodeType("motor/ForceMotor", ForceMotor);

    // Rotate motor
    function RotateMotor()
    {
        this.addInput("Dependency", "string");
        this.addOutput("MotorName", "string");
        this.properties = {
            name: "defaultRotate", 
            dependencies: [],
            selectionName: "",
            px: 0.0,
            py: 0.0,
            pz: 0.0,
            ax: 0.0,
            ay: 0.0,
            ax: 0.0,
            period: 0.0, 
            angle: 0.0,
            valid: true, 
            errorMsg: "OK"
        };

        this.widgetName = this.addWidget("text", "Name", this.properties.name, {property: "name"});
        this.widgetSelection = this.addWidget("text", "Selection", this.properties.selectionName, {property: "selectionName"});
        this.widgetPX = this.addWidget("number", "Centroid X (distance unit)", this.properties.px, {property: "px", min: -10000000000.0, max: 10000000000.0, step:1});
        this.widgetPY = this.addWidget("number", "Centroid Y (distance unit)", this.properties.py, {property: "py", min: -10000000000.0, max: 10000000000.0, step:1});
        this.widgetPZ = this.addWidget("number", "Centroid Z (distance unit)", this.properties.pz, {property: "pz", min: -10000000000.0, max: 10000000000.0, step:1});
        this.widgetAX = this.addWidget("number", "Rotation Axe x", this.properties.ax, {property: "ax", min: -10000000000.0, max: 10000000000.0, step:1});
        this.widgetAY = this.addWidget("number", "Rotation Axe y", this.properties.ay, {property: "ay", min: -10000000000.0, max: 10000000000.0, step:1});
        this.widgetAZ = this.addWidget("number", "Rotation Axe z", this.properties.az, {property: "az", min: -10000000000.0, max: 10000000000.0, step:1});
        this.widgetPeriod = this.addWidget("number", "Time of full rotation (time unit)", this.properties.period, {property: "period", min: 0.0, max: 10000000000.0, step:1});
        this.widgetAngle = this.addWidget("number", "Rotation angle (deg)", this.properties.angle, {property: "angle", min: 0.0, max: 10000000000.0, step:1})
        this.size = [400, 300];
    }

    RotateMotor.title = "Rotate Motor"
    RotateMotor.prototype.onExecute = function(){
        this.properties.dependencies = [this.getInputData(0)];
        if(this.properties.name.length <= 0)
        {
            this.properties.valid = false;
            this.properties.error = "The name of a motor cannot be empty.";
        }
        if(this.properties.selectionName.length <= 0)
        {
            this.properties.valid = false;
            this.properties.error = "The name of a selection cannot be empty.";
        }

        this.setOutputData(0, this.properties.name);
    }

    RotateMotor.prototype.onGetRadahnDict = function(motorArray)
    {
        result = {
            "type": "rotate",
            "dependencies": this.properties.dependencies,
            "name": this.properties.name,
            "selectionName" : this.properties.selectionName,
            "px": this.properties.px,
            "py": this.properties.py,
            "pz": this.properties.pz,
            "ax": this.properties.ax,
            "ay": this.properties.ay,
            "az": this.properties.az,
            "period": this.properties.period,
            "requestedAngle": this.properties.angle
        };

        motorArray.push(result);
    }

    LiteGraph.registerNodeType("motor/RotateMotor", RotateMotor);

    // Torque motor
    function TorqueMotor()
    {
        this.addInput("Dependency", "string");
        this.addOutput("MotorName", "string");
        this.properties = {
            name: "defaultTorque", 
            dependencies: [],
            selectionName: "",
            tx: 0.0,
            ty: 0.0,
            tz: 0.0,
            angle: 0.0,
            valid: true, 
            errorMsg: "OK"
        };

        this.widgetName = this.addWidget("text", "Name", this.properties.name, {property: "name"});
        this.widgetSelection = this.addWidget("text", "Selection", this.properties.selectionName, {property: "selectionName"});
        this.widgetTX = this.addWidget("number", "Torque X (torque unit)", this.properties.tx, {property: "tx", min: -10000000000.0, max: 10000000000.0, step:1});
        this.widgetTY = this.addWidget("number", "Torque Y (torque unit)", this.properties.ty, {property: "ty", min: -10000000000.0, max: 10000000000.0, step:1});
        this.widgetTZ = this.addWidget("number", "Torque Z (torque unit)", this.properties.tz, {property: "tz", min: -10000000000.0, max: 10000000000.0, step:1});
        this.widgetAngle = this.addWidget("number", "Rotation angle (deg)", this.properties.angle, {property: "angle", min: 0.0, max: 10000000000.0, step:1})
        this.size = [400, 180];
    }

    TorqueMotor.title = "Torque Motor"
    TorqueMotor.prototype.onExecute = function(){
        this.properties.dependencies = [this.getInputData(0)];
        if(this.properties.name.length <= 0)
        {
            this.properties.valid = false;
            this.properties.error = "The name of a motor cannot be empty.";
        }
        if(this.properties.selectionName.length <= 0)
        {
            this.properties.valid = false;
            this.properties.error = "The name of a selection cannot be empty.";
        }

        this.setOutputData(0, this.properties.name);
    }

    TorqueMotor.prototype.onGetRadahnDict = function(motorArray)
    {
        result = {
            "type": "torque",
            "dependencies": this.properties.dependencies,
            "name": this.properties.name,
            "selectionName" : this.properties.selectionName,
            "tx": this.properties.tx,
            "ty": this.properties.ty,
            "tz": this.properties.tz,
            "requestedAngle": this.properties.angle
        };

        motorArray.push(result);
    }

    LiteGraph.registerNodeType("motor/TorqueMotor", TorqueMotor);
}

radahnGraphUtil.generateJSON = function(lGraph)
{
    let nodes = lGraph._nodes;
    console.log("List of motors:");
    nodesArray = []
    nodes.forEach(node => {
        node.onGetRadahnDict(nodesArray);
    });

    console.log(nodesArray);
}