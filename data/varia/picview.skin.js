///////////////////////////////////////////////
////////// Picview skin definition file    ////
///////////////////////////////////////////////

// Load the AWS2 default widgets and styles.
Sys.Load("/aws/Aws2Default.js");

// Load the skin
Sys.Load("/aws/skins/Amalgam.js");

// Make the default font larger
skin_Amalgam.Font = new Font("VeraSans", 12);
skin_Amalgam.TitleFont = new Font("VeraSans", 18);

// Set it as the default skin
Skin.current = skin_Amalgam;

// We use delayed command so that we can have enough time for the loading blurb to display
function delayedCmd(cmd_code)
{		
		function fireTimer()
		{
			picView.onEvent(cmd_code);
			lw.Hide();
			return false;	
		} 
		
		Sys.CreateTimer(250, fireTimer);	
}

// Setup signal handlers for the picView object.
picView.First = function() { nmp.Hide(); lw.Show(); delayedCmd(picView.cmdFirst);  }
picView.Prev  = function() { lw.Show(); delayedCmd(picView.cmdPrev); }
picView.Next  = function() { lw.Show(); delayedCmd(picView.cmdNext); }
picView.Quit  = function() { picView.onEvent(picView.cmdQuit); }
picView.Scale = function() { lw.Show(); delayedCmd(picView.cmdScale); }

// Create a new widget with the picview controls in it.
function PicViewControls()
{
	var button_bar = 	
	   <widget type="Panel"> 	  	 
	    <setup>widget.onDraw=Skin.current.Style.RoundedPanel</setup>
		 <layout type="Horizontal" id="button_bar">		  
		 	<padding>2</padding>
		 
		 	<widget type="ButtonWithText">		 				 		
		 	    <padding>5</padding>
		 		<text>First</text>
		 		<event name="onMouseClick" action="picView.First" />		 		
		 	</widget>
		 	<widget type="ButtonWithText">		 				 		
		 		<padding>5</padding>
		 		<text>Prev</text>
		 		<event name="onMouseClick" action="picView.Prev" />		 		
		 	</widget>
		 	<widget type="ButtonWithText">		 				 		
		 		<padding>5</padding>
		 		<text>Next</text>			 		
		 		<event name="onMouseClick" action="picView.Next" />		 		
		 	</widget>
		 	<widget type="ButtonWithText" id="bb_quit">		 				 		
		 		<padding>5</padding>
		 		<text>Quit</text>
		 		<event name="onMouseClick" action="picView.Quit" />		 				 		
		 	</widget>		 	
		 	<widget type="ButtonWithText">		 				 		
		 		<padding>5</padding>
		 		<text>Scale</text>
		 		<event name="onMouseClick" action="picView.Scale" />		 				 		
		 	</widget>		 	
		 </layout>
		</widget>;	
	
	return ParseXMLInterface(button_bar, null);
}

// Create a new widget with the picview controls in it.
function NoMorePics()
{
	var label_thing = 	 	  	 
		 <widget type="ToolTip">
		 	<setup>widget.SetFocusPoint((bb_quit.xmin + (bb_quit.width>>1))+controls.xmin, controls.ymax);</setup>
		 	<title>No More Pictures</title>
		 	<text>None of the files left in this directory
is an image that this program understands.</text>	 	
		 </widget>;
	
	return ParseXMLInterface(label_thing, null);
}


// Create a new widget with the picview controls in it.
function LoadingPic()
{
	var loading_info= 	 
	  <widget type="Panel" id="loading_panel">
	    <setup>widget.onDraw=Skin.current.Style.RoundedPanel</setup>
		<layout type="Horizontal" id="loading_thing">		  
		 	<padding>2</padding>	  	 
		 	<widget type="Image">
		 		<image>/varia/loading-icon.png</image>
		 	</widget>
		 	<widget type="Label">
		 		<text>Loading image, please wait...</text>
		 	</widget>		 	
		 </layout>
		</widget>;
	
	return ParseXMLInterface(loading_info, null);
}

// Create and center loading widget.
lw = LoadingPic();
lw.ResizeTo(loading_thing.width+20, loading_thing.height+20);
loading_thing.Move(10,10);
lw.MoveTo((Sys.GetWidth()/2) - (lw.width/2), (Sys.GetHeight()/2) - (lw.height/2));

// Create a new controls widget
controls = PicViewControls();
button_bar.MoveTo(4,4);
controls.ResizeTo(button_bar.width+8, button_bar.height+8);

// Center the controls on the top of the screen
controls.MoveTo((Sys.GetWidth()/2) - (controls.width/2), 0);

// Show the button bar
controls.Show();

// Create a no more pics widget.
nmp = NoMorePics();


// Hide the console
Sys.SetConsoleVisible(false);

