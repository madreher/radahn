
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
        //this.addProperty("name", "defaultBlank");
        //this.addProperty("nbSteps", 100);
        this.properties = {name: "defaultBlank", nbSteps: 1000, valid: true, errorMsg: "OK"};
        this.widgetName = this.addWidget("text", "Name", this.properties.name, {property: "name"});
        this.widgetName = this.addWidget("number", "NbSteps", this.properties.nbSteps, {property: "nbSteps"}, {min: 1, max: 10000000000, step:1});
        //this.widgets_up = true;
        this.size = [300, 80];
    }
    BlankNode.title = "Blank Motor"
    BlankNode.prototype.onExecute = function(){
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
    LiteGraph.registerNodeType("motor/BlankMotor", BlankNode);
}