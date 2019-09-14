/*

(c) 2015 Brian Burling eMotimo INC


    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.



*/



void move_motors() { 
  
FloatPoint fp;
fp.x=0.0;
fp.y=0.0;
fp.z=0.0;
        

 //need this routine for both 2 and 3 point moves since 3 points can use this logic to determine aux motor progress      
   
  if (camera_fired<keyframe[0][1]) { //Leadin
    program_progress_2PT=1;
    if (DEBUG_MOTOR) Serial.print("LeadIn ");
  }
  else if (camera_fired<keyframe[0][2]) {  //Rampup
    program_progress_2PT=2;
    if (DEBUG_MOTOR) Serial.print("Rampup ");
  }
  else if (camera_fired<keyframe[0][3]) {  //Linear
    program_progress_2PT=3;
    if (DEBUG_MOTOR) Serial.print("Linear "); 
  }
  else if (camera_fired<keyframe[0][4]) {  //RampDown
    program_progress_2PT=4;
    if (DEBUG_MOTOR) Serial.print("RampDn ");
  }
  else if (camera_fired<keyframe[0][5]) {  //Leadout
    program_progress_2PT=5;
    if (DEBUG_MOTOR) Serial.print("LeadOut ");
  }
  else {
   program_progress_2PT=9;   //Finished
  } 

 if (progtype==REG2POINTMOVE || progtype==REV2POINTMOVE || progtype==AUXDISTANCE) {   //2 point moves    
        
 //figure out our move size 2 point SMS and VIDEO   
       if (intval==VIDEO_INTVAL) { //video moves
         fp.x = current_steps.x + motor_get_steps_2pt_video(0);
         fp.y = current_steps.y + motor_get_steps_2pt_video(1);
         fp.z = current_steps.z + motor_get_steps_2pt_video(2);
       }
       else {
         fp.x = current_steps.x + motor_get_steps_2pt(0);
         fp.y = current_steps.y + motor_get_steps_2pt(1);
         fp.z = current_steps.z + motor_get_steps_2pt(2);
       }
 }  //end progtype 0
  
  
if (progtype==REG3POINTMOVE|| progtype==REV3POINTMOVE) {  //3 point moves VIDEO
  
        if (camera_fired<keyframe[1][1]) { //Lead In
          program_progress_3PT=101;
          percent = 0.0;
          if (DEBUG_MOTOR) Serial.print("LeadIn;");Serial.print(percent);
        }       
        
        else if (camera_fired<keyframe[1][2]) { //First Leg
          program_progress_3PT=102;
          percent = float(camera_fired-keyframe[1][1]) / float((keyframe[1][2])-keyframe[1][1]);
          if (DEBUG_MOTOR) Serial.print("Leg 1;");Serial.print(percent);
        
        }
        else if (camera_fired<keyframe[1][3]) {  //Second Leg
          program_progress_3PT=103;
          percent = float(camera_fired-keyframe[1][2])/float(keyframe[1][3]-keyframe[1][2]);
          if (DEBUG_MOTOR) Serial.print("Leg 2;");Serial.print(percent);  
        }
        //else if (camera_fired<keyframe[3]) {  //Third Leg
        // program_progress_3PT=104;
        // percent = float(camera_fired-keyframe[2])/float(keyframe[3]-keyframe[2]);
        //  if (DEBUG_MOTOR) Serial.print("Leg 3;");Serial.print(percent);
        //}
        else if (camera_fired<keyframe[1][4]) {  //Lead Out
          program_progress_3PT=105;
          percent = 0.0;
          if (DEBUG_MOTOR) Serial.print("LeadOT;");Serial.print(percent); 
        }    
        
        else {
         program_progress_3PT=109;   //Finished
         if (DEBUG_MOTOR) Serial.print("Finished");Serial.print(percent);  
         return;
        } 
        
        if (DEBUG_MOTOR) Serial.print(";");  
       // DONT FORGET TO UPDATE REAL TIME TESTS ON RT Page


        fp.x = motor_get_steps_3pt(0);
        fp.y = motor_get_steps_3pt(1);
        fp.z = current_steps.z + motor_get_steps_2pt(2);  //use linear for this

}//end progtype 1


if (DEBUG_MOTOR) {Serial.print("Shot ");Serial.print(camera_fired);Serial.print(";");}

if (DEBUG_MOTOR) {Serial.print("B;");Serial.print(current_steps.x);Serial.print(";");Serial.print(current_steps.y);Serial.print(";");Serial.print(current_steps.z);Serial.print(";");}

set_target(fp.x,fp.y,fp.z); //we are in incremental mode to start abs is false

if (DEBUG_MOTOR) {Serial.print("D;");Serial.print(delta_steps.x);Serial.print(";");Serial.print(delta_steps.y);Serial.print(";");Serial.print(delta_steps.z);Serial.print(";");}

//calculate feedrate - update this to be dynamic based on settle window

//VIDEO Loop 2 Point

if ((progtype==REG2POINTMOVE || progtype==REV2POINTMOVE ||progtype==AUXDISTANCE) && (intval==VIDEO_INTVAL)) { // must lock this down to be only 2point, not three
  feedrate_micros = calculate_feedrate_delay_video();
  if (program_progress_2PT==3) {
      camera_fired=camera_fired+(keyframe[0][3]-keyframe[0][2]); //skip all the calcs mid motor move
  }
  if (DEBUG_MOTOR) {Serial.print("Feedrate:");Serial.print(feedrate_micros);Serial.print(";");}
  dda_move(feedrate_micros); 
}

//SMS Loop and all 3 point moves
else {
  feedrate_micros = calculate_feedrate_delay_1(); //calculates micro delay based on available move time
  if (intval!= VIDEO_INTVAL) feedrate_micros = min(abs(feedrate_micros), 2000); //get a slow move, but not too slow, give the motors a chance to rest for non video moves.
  if (DEBUG_MOTOR) {Serial.print("Feedrate:");Serial.print(feedrate_micros);Serial.print(";");}
  dda_move(feedrate_micros);
  Move_Engaged=false; //clear move engaged flag
}

if (DEBUG_MOTOR) {Serial.print("A;");Serial.print(current_steps.x);Serial.print(";");Serial.print(current_steps.y);Serial.print(";");Serial.print(current_steps.z);Serial.print(";");}

return;
}//end move motors

long motor_get_steps_2pt(int motor)  {
  
      long steps=0; //updated this and tested against 12 foot move 
      
      long cur_steps[3];
      cur_steps[0]=current_steps.x;
      cur_steps[1]=current_steps.y;
      cur_steps[2]=current_steps.z;
       
       //if (DEBUG) Serial.print(motor);Serial.print("motor ");
              
         switch (program_progress_2PT) {
            
            case 1:  //Lead In - 0 Steps
                   steps=0;
                  if (DEBUG_MOTOR)  Serial.print(steps);
            break;
            
            case 2: //RampUp 
                steps=(float)((camera_fired-lead_in)*linear_steps_per_shot[motor]/rampval);
                if (DEBUG_MOTOR)  Serial.print(steps);
            break;
            
            case 3:  // Linear portion
                steps=(float)(motor_steps_pt[2][motor]-ramp_params_steps[motor]-cur_steps[motor])/(keyframe[0][3]-camera_fired); //  Point 2 in the end point
                if (DEBUG_MOTOR)  Serial.print(steps);
             break;
            
            case 4:  // RampDown
                  steps=(float)((motor_steps_pt[2][motor]-cur_steps[motor])*2)/(keyframe[0][4]-camera_fired); // Point 2 in the end point for 2 point move
                  if (DEBUG_MOTOR) Serial.print(steps);
            break;
            
            case 5:  //Lead Out
                    steps=0;
                  if (DEBUG_MOTOR) Serial.print(steps);
            break;
            
            case 9:  //5 - finished	
            break;     
                  
         }      
      
           if (DEBUG_MOTOR) Serial.print(";");
           return(steps);  
}  


long motor_get_steps_3pt(int motor)  {
  
long steps=0; //updated this and tested against 12 foot move with and inch or so of travel with a 50:1 gear ration - needed to get above 32000 steps before overflow

long cur_steps[3];
cur_steps[0]=current_steps.x;
cur_steps[1]=current_steps.y;
cur_steps[2]=current_steps.z;
 
 //if (DEBUG) Serial.print(motor);Serial.print("motor ");
        
   switch (program_progress_3PT) {
      
     
     case 101:  //3Point Move - Lead In
            steps =0;
            if (DEBUG_MOTOR)  Serial.print(steps);
     break;
    
     case 102:  //3Point Move - First Leg - 
            steps = catmullrom(percent, motor_steps_pt[1][motor], 0.0, motor_steps_pt[1][motor], motor_steps_pt[2][motor]);
            if (DEBUG_MOTOR)  Serial.print(steps);
      break;
      
     case 103: //3Point Move - Second Leg 
            steps = catmullrom(percent, 0.0, motor_steps_pt[1][motor], motor_steps_pt[2][motor], motor_steps_pt[1][motor]);
            if (DEBUG_MOTOR)  Serial.print(steps);
      break;
      
      //case 104: //3Point Move - Third Leg 
      //    steps = catmullrom(percent, motor_steps_pt[1][motor], motor_steps_pt[2][motor], motor_steps_pt[3][motor], motor_steps_pt[2][motor]);
          
      //    if (DEBUG_MOTOR)  Serial.print(steps);
      //break;
      
      case 105: //3Point Move - Lead Out
            steps = cur_steps[motor]; //this is not delta but relative to start.
            if (DEBUG_MOTOR)  Serial.print(steps);
      break;
      
      
      case 109:  //109 - finished
      	  steps =0.0;
      break;      
        
   }      

     if (DEBUG) Serial.print(";");
     return(steps);  
}  




float catmullrom(float t, float p0, float p1, float p2, float p3)
{
    return 0.5f * (
                  (2 * p1) +
                  (-p0 + p2) * t +
                  (2 * p0 - 5 * p1 + 4 * p2 - p3) * t * t +
                  (-p0 + 3 * p1 - 3 * p2 + p3) * t * t * t
                  );
}  


long motor_get_steps_2pt_video(int motor)  {
  
      long steps=0; //updated this and tested against 12 foot move with and inch or so of travel with a 50:1 gear ration - needed to get above 32000 steps before overflow
      
      long cur_steps[3];
      cur_steps[0]=current_steps.x;
      cur_steps[1]=current_steps.y;
      cur_steps[2]=current_steps.z;
       
       //if (DEBUG) Serial.print(motor);Serial.print("motor ");
              
         switch (program_progress_2PT) {
            
            case 1:  //Lead In - 0 Steps
                   steps=0;
                  if (DEBUG_MOTOR)  Serial.print(steps);
            break;
            
            case 2: //RampUp 
                steps=(float)((camera_fired-lead_in)*linear_steps_per_shot[motor]/rampval);
                if (DEBUG_MOTOR)  Serial.print(steps);
            break;
            
            case 3:  // Linear portion
                //steps=(float)(motor_steps_pt[2][motor]-ramp_params_steps[motor]-cur_steps[motor])/(keyframe[0][3]-camera_fired); //  Point 2 in the end point  //THIS IS THE OLD ROUTINE
                steps=(motor_steps_pt[2][motor]-ramp_params_steps[motor]-cur_steps[motor]); //  This is total steps of the linear portion.  End point  - ramp down-where we are
                if (DEBUG_MOTOR)  Serial.print(steps);
             break;
            
            case 4:  // RampDown
                  steps=(float)((motor_steps_pt[2][motor]-cur_steps[motor])*2)/(keyframe[0][4]-camera_fired); // Point 2 in the end point for 2 point move
                  if (DEBUG_MOTOR) Serial.print(steps);
            break;
            
            case 5:  //Lead Out
                    steps=0;
                  if (DEBUG_MOTOR) Serial.print(steps);
            break;
            
            case 9:  //5 - finished	
            break;     
                  
         }      
      
           if (DEBUG_MOTOR) Serial.print(";");
           return(steps);  
}  

void go_to_origin_max_speed() // interrupt routine
{
          
     
      if (DEBUG_MOTOR) Serial.print("motors[0].dest:"); Serial.println( motors[0].destination);
      if (DEBUG_MOTOR) Serial.print("motors[1].dest:"); Serial.println( motors[1].destination);
      if (DEBUG_MOTOR) Serial.print("motors[2].dest:"); Serial.println( motors[2].destination);
    
   
     synched3PtMove_max(0.0, 0.0, 0.0); 
     

      if (DEBUG_MOTOR) Serial.print("motors[0].position:"); Serial.println( motors[0].position);
      if (DEBUG_MOTOR) Serial.print("motors[1].position:"); Serial.println( motors[1].position);
      if (DEBUG_MOTOR) Serial.print("motors[2].position:"); Serial.println( motors[2].position);
      
      if (DEBUG_MOTOR) Serial.print("motors[0].dest:"); Serial.println( motors[0].destination);
      if (DEBUG_MOTOR) Serial.print("motors[1].dest:"); Serial.println( motors[1].destination);
      if (DEBUG_MOTOR) Serial.print("motors[2].dest:"); Serial.println( motors[2].destination);
      
           
      //bitSet(motorMoving, 0);
      //bitSet(motorMoving, 1);
      //bitSet(motorMoving, 2);
     
     

      startISR1 ();
      do 
      {
        if (!nextMoveLoaded)
        {
          updateMotorVelocities();
        }
      } while (motorMoving);
      //delay(10000);
      stopISR1 ();
      
      

      //this is making sure position is keeping up and accurate with granular steps
      if (DEBUG_MOTOR) Serial.print("Mot 0 current steps after move:"); Serial.println( current_steps.x);
      if (DEBUG_MOTOR) Serial.print("Mot 1 current steps after move:"); Serial.println( current_steps.y);
      if (DEBUG_MOTOR) Serial.print("Mot 2 current steps after move:"); Serial.println( current_steps.z);
      if (DEBUG_MOTOR) Serial.print("motors[0].position:"); Serial.println( motors[0].position);
      if (DEBUG_MOTOR) Serial.print("motors[1].position:"); Serial.println( motors[1].position);
      if (DEBUG_MOTOR) Serial.print("motors[2].position:"); Serial.println( motors[2].position);
      

  }
  
  
  
void go_to_origin_slow() // interrupt routine
{
          
     
      if (DEBUG_MOTOR) Serial.print("motors[0].dest:"); Serial.println( motors[0].destination);
      if (DEBUG_MOTOR) Serial.print("motors[1].dest:"); Serial.println( motors[1].destination);
      if (DEBUG_MOTOR) Serial.print("motors[2].dest:"); Serial.println( motors[2].destination);
    
   //synched3PtMove_timed(0.0, 0.0, 0.0,15.0,0.25);
    synched3PtMove_max(0.0, 0.0, 0.0); 
     

      if (DEBUG_MOTOR) Serial.print("motors[0].position:"); Serial.println( motors[0].position);
      if (DEBUG_MOTOR) Serial.print("motors[1].position:"); Serial.println( motors[1].position);
      if (DEBUG_MOTOR) Serial.print("motors[2].position:"); Serial.println( motors[2].position);
      
      if (DEBUG_MOTOR) Serial.print("motors[0].dest:"); Serial.println( motors[0].destination);
      if (DEBUG_MOTOR) Serial.print("motors[1].dest:"); Serial.println( motors[1].destination);
      if (DEBUG_MOTOR) Serial.print("motors[2].dest:"); Serial.println( motors[2].destination);
      
           
      //bitSet(motorMoving, 0);
      //bitSet(motorMoving, 1);
      //bitSet(motorMoving, 2);
     
     

      startISR1 ();
      do 
      {
        if (!nextMoveLoaded)
        {
          updateMotorVelocities();
        }
      } while (motorMoving);
      //delay(10000);
      stopISR1 ();
      
      

      //this is making sure position is keeping up and accurate with granular steps
      if (DEBUG_MOTOR) Serial.print("Mot 0 current steps after move:"); Serial.println( current_steps.x);
      if (DEBUG_MOTOR) Serial.print("Mot 1 current steps after move:"); Serial.println( current_steps.y);
      if (DEBUG_MOTOR) Serial.print("Mot 2 current steps after move:"); Serial.println( current_steps.z);
      if (DEBUG_MOTOR) Serial.print("motors[0].position:"); Serial.println( motors[0].position);
      if (DEBUG_MOTOR) Serial.print("motors[1].position:"); Serial.println( motors[1].position);
      if (DEBUG_MOTOR) Serial.print("motors[2].position:"); Serial.println( motors[2].position);
      

  }  
  
  



void DisplayMove(int motorIndex) //ca
{
  Motor *motor = &motors[motorIndex];
  
  int i, moveCount;
  moveCount = 0;

  for (i = 0; i < 5; i++)
  {
    if (DEBUG_MOTOR) Serial.print("M");Serial.print(motorIndex);Serial.print("Seg:");Serial.print(i);
    if (DEBUG_MOTOR) Serial.print("T:");Serial.print(motor->moveTime[i]);Serial.print(",");
    if (DEBUG_MOTOR) Serial.print("P:");Serial.print(motor->movePosition[i]);Serial.print(",");
    if (DEBUG_MOTOR) Serial.print("V:");Serial.print(motor->moveVelocity[i]);Serial.print(",");
    if (DEBUG_MOTOR) Serial.print("A:");Serial.print(motor->moveAcceleration[i]);Serial.print(",");
    if (DEBUG_MOTOR) Serial.print("Dest:");Serial.print(motor->destination);Serial.println(" ");

  }
  //Serial.print("Tmax:");Serial.println(tmax);
  //Serial.print("Dmax:");Serial.println(dmax);
  //Serial.print("Dist:");Serial.println(dist);
  //Serial.print("Dir:");Serial.println(dir);
    
}



void go_to_start_old(){
  
  

        // This is the first time we look at movement.  Here is where we need to update our positions to accomodate reverse programming.  
        // Goal here is to just reverse the stored targets.  These are floating point so we should be able to just multiply by -1 - verify with debug.  
        // Additionally we have to set zeros for the current position - the move to start should do nothing.
        
        if (REVERSE_PROG_ORDER&&!MOVE_REVERSED_FOR_RUN) 
        {
              
               MOVE_REVERSED_FOR_RUN=true;
              //first flip the stored variables to help with direction of the move
              if (DEBUG_MOTOR)  Serial.print("entering rev loop of interest");
              if (DEBUG_MOTOR) Serial.println(REVERSE_PROG_ORDER);
              
              //if we aren't at the end position, move there now
              //  FloatPoint fp;
              //    fp.x = motor_steps_pt[2][0];
              //    fp.y = motor_steps_pt[2][1];
              //    fp.z = motor_steps_pt[2][2];
              // set_target(fp.x, fp.y, fp.z);
              // lcd.at(1,1,"Going to End Pt."); 
              // dda_move(10);
               //end of move to end position
              
               //calc the move
               synched3PtMove_max(motor_steps_pt[2][0],motor_steps_pt[2][1],motor_steps_pt[2][2]);

                //Start us moving  
                startISR1 ();
                

                stopISR1 ();
                //Clean up positions so we don't drift
                motors[0].position = long(current_steps.x);
                motors[1].position = long(current_steps.y);
                motors[2].position = long(current_steps.z);
               
               
               
               
               
              
               if (DEBUG) {Serial.print("midpoint_preflip");Serial.print(motor_steps_pt[1][0]);Serial.print(",");Serial.print(motor_steps_pt[1][1]);Serial.print(",");Serial.print(motor_steps_pt[1][2]);Serial.println(); }              
                 motor_steps_pt[1][0] = (motor_steps_pt[2][0]-motor_steps_pt[1][0])*-1.0;
                 motor_steps_pt[1][1] = (motor_steps_pt[2][1]-motor_steps_pt[1][1])*-1.0;
                 motor_steps_pt[1][2] = (motor_steps_pt[2][2]-motor_steps_pt[1][2])*-1.0;

              if (DEBUG) {Serial.print("endpoint_preflip");Serial.print(motor_steps_pt[2][0]);Serial.print(",");Serial.print(motor_steps_pt[2][1]);Serial.print(",");Serial.print(motor_steps_pt[2][2]);Serial.println();  }            
                 motor_steps_pt[2][0] *=-1.0;
                 motor_steps_pt[2][1] *=-1.0;
                 motor_steps_pt[2][2] *=-1.0;
              if (DEBUG) {Serial.print("endpoint_postflip");Serial.print(motor_steps_pt[2][0]);Serial.print(",");Serial.print(motor_steps_pt[2][1]);Serial.print(",");Serial.print(motor_steps_pt[2][2]);Serial.println(); }
              
              //we need to figure out where we are before we jump home, can't assume the correct end point.
              set_position(0.0, 0.0, 0.0); //setting home
        
        }


        if (progtype==REG2POINTMOVE || progtype==REV2POINTMOVE || progtype==AUXDISTANCE){ //2point move
           for (int i=0; i < MOTORS; i++){
               linear_steps_per_shot[i] = motor_steps_pt[2][i]/(camera_moving_shots-rampval); //This assumes ramp is equal on either side   SIGNED!!!
               if (DEBUG_MOTOR) {Serial.print("LinSteps/Shot_");Serial.print(i);Serial.print("_");Serial.println(linear_steps_per_shot[i]);}
           }
           
           //This is to calc the steps at the end of rampup for each motor.  Each array value is for a motor                   
           
           for (int i=0; i < MOTORS; i++){
               ramp_params_steps[i] = 0.5*rampval*linear_steps_per_shot[i]; //steps at end of ramping target
               if (DEBUG_MOTOR) {Serial.print("Ramp_Steps_");Serial.print(i);Serial.print("_");Serial.println(ramp_params_steps[i]);}
           }
       }
      
      
      
      
       if (progtype==REG3POINTMOVE || progtype==REV3POINTMOVE){ //3point move
       
         
         keyframe[1][0]=0; //start frame - must find and replace this 
         keyframe[1][1]=lead_in; //end of static, start of leg 1.
         keyframe[1][2]=(camera_moving_shots/(MAX_MOVE_POINTS-1))+lead_in; //end of leg 1, start of leg 2
         keyframe[1][3]=camera_moving_shots+lead_in; //end of leg 2, start of lead_out
         keyframe[1][4]=camera_moving_shots+lead_in+lead_out; //end of lead_out/shot
         
         /*
         keyframe[1][0]=0; //start frame
         keyframe[1][1]=camera_moving_shots/(MAX_MOVE_POINTS-1); //mid frame 
         keyframe[1][2]=camera_moving_shots*2/(MAX_MOVE_POINTS-1); //end frame
         */
         
         //keyframe[3]=camera_moving_shots; //end point
         
          motor_steps_pt[1][2] = motor_steps_pt[2][2];  //overwrite the middle points for the aux - not needed here and used for linear calcs on motor control 
         
         for (int i=2; i < MOTORS; i++){ //overwritw for just the aux motor using second point
               linear_steps_per_shot[i] = motor_steps_pt[2][i]/(camera_moving_shots-rampval); //This assumes ramp is equal on either side   SIGNED!!!
               if (DEBUG_MOTOR) {Serial.print("LinSteps/Shot_");Serial.print(i);Serial.print("_");Serial.println(linear_steps_per_shot[i]);}
           }
           
           //This is to calc the steps at the end of rampup for each motor.  Each array value is for a motor                   
           
         for (int i=2; i < MOTORS; i++){ //overwritw for just the aux motor using second point
               ramp_params_steps[i] = 0.5*rampval*linear_steps_per_shot[i]; //steps at end of ramping target
               if (DEBUG_MOTOR) {Serial.print("Ramp_Steps_");Serial.print(i);Serial.print("_");Serial.println(ramp_params_steps[i]);}
           }
       
         
      } //end of three point calcs
     
 
       delay (prompt_time);
       draw(40,1,1);//lcd.at(1,1," Going to Start"); //Moving back to start point
       digitalWrite(MS1, HIGH); //ensure microstepping before jog back home
       digitalWrite(MS2, HIGH);
       digitalWrite(MS3, HIGH);
       
       
       //Sync DF positions variables to floating points
       motors[0].position = current_steps.x;
       motors[1].position = current_steps.y;
       motors[2].position = current_steps.z;
       
       enable_PT();
       enable_AUX();
       go_to_origin_max_speed();

  }
  
  
  
  
void go_to_start_new() // interrupt routine
{
      enable_PT();
      enable_AUX();    
     //Add Section to allow for reverse Stuff.
      
  
        // This is the first time we look at movement.  Here is where we need to update our positions to accomodate reverse programming.  
        // Goal here is to just reverse the stored targets.  These are floating point so we should be able to just multiply by -1 - verify with debug.  
        // Additionally we have to set zeros for the current position - the move to start should do nothing.
        
        if (REVERSE_PROG_ORDER&&!MOVE_REVERSED_FOR_RUN) 
        {
              
              MOVE_REVERSED_FOR_RUN=true;
              //first flip the stored variables to help with direction of the move
              if (DEBUG) Serial.print("entering rev loop");
              if (DEBUG) Serial.println(REVERSE_PROG_ORDER);
              
              //if we aren't at the end position, move there now
              //  FloatPoint fp;
              //    fp.x = motor_steps_pt[2][0];
              //    fp.y = motor_steps_pt[2][1];
              //    fp.z = motor_steps_pt[2][2];
              // set_target(fp.x, fp.y, fp.z);
              // lcd.at(1,1,"Going to End Pt."); 
              // dda_move(10);
               //end of move to end position
              
               //calc the move
               synched3PtMove_max(motor_steps_pt[2][0],motor_steps_pt[2][1],motor_steps_pt[2][2]);

                //Start us moving  
                startISR1 ();
                do 
                {
                  if (!nextMoveLoaded)
                  {
                    updateMotorVelocities();
                  }
                } while (motorMoving);
                //delay(10000);
                stopISR1 ();
                
                
                //Clean up positions so we don't drift
                motors[0].position = long(current_steps.x);
                motors[1].position = long(current_steps.y);
                motors[2].position = long(current_steps.z);
               
               
              
               if (DEBUG) {Serial.print("midpoint_preflip:");Serial.print(motor_steps_pt[1][0]);Serial.print(",");Serial.print(motor_steps_pt[1][1]);Serial.print(",");Serial.print(motor_steps_pt[1][2]);Serial.println(); }              
                 motor_steps_pt[1][0] = (motor_steps_pt[2][0]-motor_steps_pt[1][0])*-1.0;
                 motor_steps_pt[1][1] = (motor_steps_pt[2][1]-motor_steps_pt[1][1])*-1.0;
                 motor_steps_pt[1][2] = (motor_steps_pt[2][2]-motor_steps_pt[1][2])*-1.0;

              if (DEBUG) {Serial.print("endpoint_preflip:");Serial.print(motor_steps_pt[2][0]);Serial.print(",");Serial.print(motor_steps_pt[2][1]);Serial.print(",");Serial.print(motor_steps_pt[2][2]);Serial.println();  }            
                 motor_steps_pt[2][0] *=-1.0;
                 motor_steps_pt[2][1] *=-1.0;
                 motor_steps_pt[2][2] *=-1.0;
              if (DEBUG) {Serial.print("endpoint_postflip:");Serial.print(motor_steps_pt[2][0]);Serial.print(",");Serial.print(motor_steps_pt[2][1]);Serial.print(",");Serial.print(motor_steps_pt[2][2]);Serial.println(); }
              
              //we need to figure out where we are before we jump home, can't assume the correct end point.
              set_position(0.0, 0.0, 0.0); //setting home
        
        }
  
  
  
  
  
  
  
      if (DEBUG_MOTOR) Serial.print("motors[0].dest:"); Serial.println( motors[0].destination);
      if (DEBUG_MOTOR) Serial.print("motors[1].dest:"); Serial.println( motors[1].destination);
      if (DEBUG_MOTOR) Serial.print("motors[2].dest:"); Serial.println( motors[2].destination);
    
     //synched3PtMove_timed(0.0, 0.0, 0.0,15.0,0.25);
     //need to turn on the motors
      synched3PtMove_max(0.0, 0.0, 0.0); 
     

      if (DEBUG_MOTOR) Serial.print("motors[0].position:"); Serial.println( motors[0].position);
      if (DEBUG_MOTOR) Serial.print("motors[1].position:"); Serial.println( motors[1].position);
      if (DEBUG_MOTOR) Serial.print("motors[2].position:"); Serial.println( motors[2].position);
      
      if (DEBUG_MOTOR) Serial.print("motors[0].dest:"); Serial.println( motors[0].destination);
      if (DEBUG_MOTOR) Serial.print("motors[1].dest:"); Serial.println( motors[1].destination);
      if (DEBUG_MOTOR) Serial.print("motors[2].dest:"); Serial.println( motors[2].destination);
      
           
      //bitSet(motorMoving, 0);
      //bitSet(motorMoving, 1);
      //bitSet(motorMoving, 2);
      draw(40,1,1);//lcd.at(1,1," Going to Start"); //Moving back to start point



      startISR1 ();
      do 
      {
        if (!nextMoveLoaded)
        {
          updateMotorVelocities();
        }
      } while (motorMoving);
      //delay(10000);
      stopISR1 ();
      
      

      //this is making sure position is keeping up and accurate with granular steps
      if (DEBUG_MOTOR) Serial.print("Mot 0 current steps after move:"); Serial.println( current_steps.x);
      if (DEBUG_MOTOR) Serial.print("Mot 1 current steps after move:"); Serial.println( current_steps.y);
      if (DEBUG_MOTOR) Serial.print("Mot 2 current steps after move:"); Serial.println( current_steps.z);
      if (DEBUG_MOTOR) Serial.print("motors[0].position:"); Serial.println( motors[0].position);
      if (DEBUG_MOTOR) Serial.print("motors[1].position:"); Serial.println( motors[1].position);
      if (DEBUG_MOTOR) Serial.print("motors[2].position:"); Serial.println( motors[2].position);
      

       //still missing the reverse, but this calcs for ramp are now in.
       
       
        if (progtype==REG2POINTMOVE || progtype==REV2POINTMOVE || progtype==AUXDISTANCE){ //2point move
           for (int i=0; i < MOTORS; i++){
               linear_steps_per_shot[i] = motor_steps_pt[2][i]/(camera_moving_shots-rampval); //This assumes ramp is equal on either side   SIGNED!!!
               if (DEBUG_MOTOR) {Serial.print("LinSteps/Shot_");Serial.print(i);Serial.print("_");Serial.println(linear_steps_per_shot[i]);}
           }
           
           //This is to calc the steps at the end of rampup for each motor.  Each array value is for a motor                   
           
           for (int i=0; i < MOTORS; i++){
               ramp_params_steps[i] = 0.5*rampval*linear_steps_per_shot[i]; //steps at end of ramping target
               if (DEBUG_MOTOR) {Serial.print("Ramp_Steps_");Serial.print(i);Serial.print("_");Serial.println(ramp_params_steps[i]);}
           }
       }
      
      
      
      
       if (progtype==REG3POINTMOVE || progtype==REV3POINTMOVE){ //3point move
       
         
         keyframe[1][0]=0; //start frame - must find and replace this 
         keyframe[1][1]=lead_in; //end of static, start of leg 1.
         keyframe[1][2]=(camera_moving_shots/(MAX_MOVE_POINTS-1))+lead_in; //end of leg 1, start of leg 2
         keyframe[1][3]=camera_moving_shots+lead_in; //end of leg 2, start of lead_out
         keyframe[1][4]=camera_moving_shots+lead_in+lead_out; //end of lead_out/shot
         
         /*
         keyframe[1][0]=0; //start frame
         keyframe[1][1]=camera_moving_shots/(MAX_MOVE_POINTS-1); //mid frame 
         keyframe[1][2]=camera_moving_shots*2/(MAX_MOVE_POINTS-1); //end frame
         */
         
         //keyframe[3]=camera_moving_shots; //end point
         
          motor_steps_pt[1][2] = motor_steps_pt[2][2];  //overwrite the middle points for the aux - not needed here and used for linear calcs on motor control 
         
         for (int i=2; i < MOTORS; i++){ //overwritw for just the aux motor using second point
               linear_steps_per_shot[i] = motor_steps_pt[2][i]/(camera_moving_shots-rampval); //This assumes ramp is equal on either side   SIGNED!!!
               if (DEBUG_MOTOR) {Serial.print("LinSteps/Shot_");Serial.print(i);Serial.print("_");Serial.println(linear_steps_per_shot[i]);}
           }
           
           //This is to calc the steps at the end of rampup for each motor.  Each array value is for a motor                   
           
         for (int i=2; i < MOTORS; i++){ //overwritw for just the aux motor using second point
               ramp_params_steps[i] = 0.5*rampval*linear_steps_per_shot[i]; //steps at end of ramping target
               if (DEBUG_MOTOR) {Serial.print("Ramp_Steps_");Serial.print(i);Serial.print("_");Serial.println(ramp_params_steps[i]);}
           }
       
         
      } //end of three point calcs
      

      
      

//We don't know how log we will be waiting - go to powersave.
      if (POWERSAVE_PT>2 && (sequence_repeat_type!=0))   disable_PT(); //don't powersave for continuous
      if (POWERSAVE_AUX>2 && (sequence_repeat_type!=0))   disable_AUX(); //don't powersave for continuous

  }  
  
  
