/* @file New Keypad
 || @version 15.7
 || @author Sam Bozman
 || @contact s.j.Bozman@gmail.com
 ||
 || @description
 || Garage door KeyPad routines
 */
#include <Keypad.h>
#include <EEPROM.h>

//********** Custom variables *********************************************
int pinPointer = 0; //initialize pinPointer variable
const int pinPointerSave = 250; //Save the pin pointer at EEProm address 250
String pinCode =  ""; //variable to hold Pin Codes (4) digits)
String temp = ""; //temporary variable used to compare strings
char pinChar; //Used to read individual characters from EEPROM
char mf = 'X'; //Used to hold Master Function code ('E' or '?')
char key; //Used to read in keypad key
boolean result; //Used to indicate true or false
int piezoPin = 10;  //pin for piezo transducer
int redLed = 11;  // Red LED pin
int doorPin = 12;  // Door pin
int greenLed = 13;  // Green LED pin
int frequency = 1047; // frequency of piezo sound
int gC = 1;
int rC = 0;
int gT = 0;
int rT = 0;
int ledState = LOW;             // ledState used to set the LED
long previousMillis = 0;        // will store last time LED was updated
long interval = 500;           // interval at which to blink (milliseconds)


const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
//define the cymbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
  {'1','2','3','U'},
  {'4','5','6','D'},
  {'7','8','9','*'},
  {'C','0','?','E'}
};
byte rowPins[ROWS] = {2, 3, 4, 5}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {6, 7, 8, 9}; //connect to the column pinouts of the keypad

//initialize an instance of class NewKeypad
Keypad garageKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

void setup()  //******************************************************************
{ 
  Serial.begin(9600);

  pinMode(doorPin, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(redLed, OUTPUT);
  pinMode(piezoPin, OUTPUT);


  digitalWrite(doorPin, LOW);    // make sure door switch is off!
  digitalWrite(greenLed, LOW); //turn green led off
  digitalWrite(redLed, LOW);  //turn red led off
  status(1);//set current status to "OK"
  garageKeypad.setDebounceTime(50);           // Default is 50mS
  initializeMaster(); //set Master pinCode to '1234' if there is no existing Master code
}  //END setup()



void loop()
{
  char key = garageKeypad.getKey();
  
  if (key)//If a key press was detected
  {
     switch (key) //Check to see what was pressed
      {
      
        case '*'://Erase all Pin Codes
              Serial.println(key);
                
                switch (mf)//Test value of Master Flag ('E' or '?' or 'X') 
                  {
                      case 'X':
                          if (checkMaster(pinCode) == true)//is this the Master Pin?
                             {
                               Serial.println("All user Pin Codes erased!"); 
                               EEPROM.write(pinPointerSave,0);//Erase User Pin Codes 
                               status(1); 
                             }//END if (checkMaster(pinCode) == true)
                          else status(05); // Master Pin Code is NOT Valid!
                      break;

                      default: // Was not 'X'
                            // if nothing else matches
                            status(10); // Unknown Error
                        break;
                    }//END switch (mf)             
        break;//case '*'
               
        case 'E'://Add a new User pin code - Key Pressed was 'E' 
              Serial.println(key);
                
                switch (mf)//Test value of Master Flag ('X' or 'E') 
                  {
                    case 'X':
                      if (checkMaster(pinCode) == true)//is the Master code valid?
                         {
                           Serial.println("Master pin code is valid"); 
                           mf = 'E';//Set Master flag  
                      }//END if (checkMaster(pinCode) == true)
                    break;
                    
                    case 'E'://Master has already been verified
                      //Check that new pin code length is correct
                      //and if OK then add the new pin Code
                      if (pinCode.length()== 4) addPinCode(pinCode);
    
                      else if (pinCode.length()!=4) status(3);
                      break;
                    
                    default: 
                      // if nothing else matches
                      status(10); // Unknown Error
                      break;
                    }
            
            break;//case 'E'
        
        case '?': // Add a new Master pin code. - Key pressed was '?' (2ND)  
               Serial.println(key);
                
                switch (mf)//Test value of Master Flag ('E' or '?' or 'X') 
                  {
                    case 'X':
                      if (checkMaster(pinCode) == true)//is the Master code valid?
                         {
                           Serial.println("Master pin code is valid"); 
                           mf = '?';//Set Master flag  
                          }//END if (checkMaster(pinCode) == true)
                      else status(05); // Master Pin Code is NOT Valid!
                    break;
                    
                    case '?'://Master has already been verified
                      //Check that new pin code length is correct
                      //and if OK then add the new pin Code
                      if (pinCode.length()== 4) addNewMaster(pinCode);
    
                      else if (pinCode.length()!=4) status(3);
                      break;
                    
                    default: 
                      // if nothing else matches
                      status(10); // Unknown Error
                      break;
                    }
             
              break;//case '?'
  
        case 'C'://Clear current pinCode entry and start again
              Serial.println(key);
              status(1); //Clear everything 
              break;//Case 'C'
      
  
        case 'D':
              Serial.println(key);
              //If Pin code is valid then toggle door latch
              if(checkPinCode(pinCode)) toggleDoor(); 
              else status(2); //Pin Code invalid!
              break; 
  
        case 'U':
              Serial.println(key);
              //If Pin code is valid then toggle door latch
              if(checkPinCode(pinCode)) toggleDoor(); 
              else status(2); //Pin Code invalid!
              break; 
      
        default: //If none of the above then must be a digit but we will double check
              if (key >= 0x30 && key <= 0x39) processDigit(key);//If key is between 0 and 9 
             
             else status(10); //Unknown Error
              
              break;
      }//END switch (key)        
   }//END if (key)
   processTimers();// No key press detected so we will process the timer
}//END loop

//Add a New Master pin code******************************************************
void addNewMaster(String pinIn)
{
  if(!checkPinCode(pinIn))
    {
        for (int i = 0; i < 4; i++)
          {   
            EEPROM.write(i+4,pinCode.charAt(i));
          }//END for 
       Serial.println("New Master code was successfully added!");
       status(1);    
    }//END if
  else status(8); //New Master matches an existing user code
}//END addNewMaster()

//Toggle door *********************************************************************
    void toggleDoor()
{
  playBeep(2,100); //Make OK sound 
  Serial.println("Pin Code was OK!: Toggling door");
    digitalWrite(doorPin, HIGH);   //Toggle door switch ON
    delay(1000); //wait 1 second
    digitalWrite(doorPin, LOW);   //Toggle door switch OFF
    status(1);
}//END toggleDoor()


// Add a new pinCode and return True or False (Success or Failure)
void addPinCode(String pinCode)  //*****************************************************
{ 
    if (!masterReplication(pinCode)) //Check that New Pin Code is not the same as the Master Code
        {
            if(!checkPinCode(pinCode)) //Check that New pin code does not already exist
                {
                    pinPointer = EEPROM.read(pinPointerSave); //Read the pinPointer (Max 10)
            	    if(pinPointer < 10)//Write pinCode to next memory spot
            	        {            		  
            		        for (int i = 0; i < 4; i++) 
                                {  
            			            EEPROM.write(((pinPointer+2)*4+i),pinCode.charAt(i));            			          
            		            }
            	            pinPointer++; //Add 1 to pinPointer
            		        EEPROM.write(pinPointerSave,pinPointer); //Write new pinPonter value back to pinPointerSave address
                            Serial.println("New user pin code successfully added");
                            status(1);
                         }	
	                else if(pinPointer >=10) status(6); // User pin codes at a maximum of 10
                }
            else status(9); //New pin code already exists!
        }
    else status(7);//New User Pin Code matches the current Master Code!
}//End addPinCode(String pinCode)

// Check user pinCode and return 'true' if it is valid or 'false' if it is not valid
boolean checkPinCode(String pinCodeIn){ //*********************************************
  temp = ""; //initialize compare variable
  pinPointer = EEPROM.read(pinPointerSave); //Get the pinPointer (0-9);
  for(int i = 0; i < pinPointer; i++ ){ // cycle through saved pin code addresses
    for (int j = 0 ; j < 4; j++){  //cycle through each digit of  current pin code
      pinChar = EEPROM.read((i+2)*4+j); //read in 4 digits into temp variable
      temp += pinChar;
    }//End for int j   
    if (temp == pinCodeIn) {
      temp = "";
      return true;
    }
    else{
      temp = "";
      
    }

  } //End for int i 
  return false; // Pin code not found
} //end function checkPinCode



//Function to check that New User pin 
//is not the same as the current Master code *******************************************
boolean masterReplication(String repCode)
{
  temp = "";
  //Get  4 char of Master code
  for (int i = 4; i < 8; i++) 
  {
    pinChar = (EEPROM.read(i));
    temp += pinChar;
  } 
  if(temp == repCode)
  {
    return true;
  }
  return false;
}



//Process current status************************************************************
void status(int statusIn)
{   
  switch (statusIn)
  {
    case 1:
      Serial.println("Everything is OK");
      gC = 1;   
      rC = 0;
      mf = 'X';   //Clear Master Flag
      pinCode = "";  //Clear pinCode
      playBeep(1,100); //Make OK sound
      break;
      
    case 2:
      Serial.println("PinCode was invalid!");
      gC = 1;   
      rC = 1;
      mf = 'X';   //Clear Master Flag
      pinCode = "";  //Clear pinCode
      playBeep(1,500); //Make Error sound
      break;
    
    case 3:
      Serial.println("Pin Code was not complete before a function key was pressed!");
      gC = 1;   
      rC = 2;
      mf = 'X';   //Clear Master Flag
      pinCode = "";  //Clear pinCode
      playBeep(1,500); //Make Error sound
      break;
      
    case 4:
      Serial.println("Expected a function key but a number was entered instead!");
      gC = 1;   
      rC = 3;
      mf = 'X';   //Clear Master Flag
      pinCode = "";  //Clear pinCode
      playBeep(1,500); //Make Error sound
      break;  
      
    case 5:
      Serial.println("Master Pin Code is NOT Valid!");
      gC = 1;   
      rC = 4;
      mf = 'X';   //Clear Master Flag
      pinCode = "";  //Clear pinCode
      playBeep(1,500); //Make Error sound
      break; 
      
    case 6:
      Serial.println("The number of existing Pin codes are at the maximum of 10");
      gC = 2;   
      rC = 1;
      mf = 'X';   //Clear Master Flag
      pinCode = "";  //Clear pinCode
      playBeep(1,500); //Make Error sound
      break; 
    
    case 7:
      Serial.println("New User Pin Code matches the current Master Code!");
      gC = 2;   
      rC = 2;
      mf = 'X';   //Clear Master Flag
      pinCode = "";  //Clear pinCode
      playBeep(1,500); //Make Error sound
      break;
    
    case 8:
      Serial.println("New Master Code matches an existing User Pin Code!");
      gC = 2;   
      rC = 3;
      mf = 'X';   //Clear Master Flag
      pinCode = "";  //Clear pinCode
      playBeep(1,500); //Make Error sound
      break;
    
    case 9:
      Serial.println("New pin code already exists!");
      gC = 2;   
      rC = 4;
      mf = 'X';   //Clear Master Flag
      pinCode = "";  //Clear pinCode
      playBeep(1,500); //Make Error sound
      break;
      
    case 10:
      Serial.println("Programming Error");
      gC = 3;   
      rC = 1;
      mf = 'X';   //Clear Master Flag
      pinCode = "";  //Clear pinCode
      playBeep(1,500); //Make Error sound
      break;

    default: 
      Serial.println("Status Error. Reason Unknown!");
      gC = 3;   
      rC = 2;
      mf = 'X';   //Clear Master Flag
      pinCode = "";  //Clear pinCode
      playBeep(1,500); //Make Error sound
      break;
  } //END Switch Case
}//END status()


//Function to check for valid MasterCode*********************************************
boolean checkMaster(String mastCode)
{
  if (pinCode.length() == 4)
    {
        temp = "";
        //Get first 4 char of EEProm memory
        for (int i = 4; i < 8; i++) 
          {
            pinChar = (EEPROM.read(i));
            temp += pinChar;
          } 
        if(temp == mastCode) 
          { 
            status(1); // Set status to 'OK'
            return true;
          }//END if(temp == mastCode) 
        
        else if (temp != mastCode) 
          {
            status(5); // Set status to 'Master Pin Code was invalid!'
            return false;
          }//END else if (temp != mastCode)
    
  }//END if (pinCode.length() == 4)
  
  else if (pinCode.length() != 4)
    {
       status(3);
        return false;
    }//END else if (pinCode.length() != 4)

}//END checkMaster()

// Creates a 'Default' master PinCode (1234) if none exists.
void initializeMaster(){ //**********************************************************************
  // initialize string variable
  pinCode = "";
  //Get first 4 char of EEProm memory
  for (int i = 0; i < 4; i++) {
    pinChar = (EEPROM.read(i));
    pinCode += pinChar;
  }
  // If masterCode does not equal 'MAST' then
  // write 'MAST1234' into first 8 bytes of EEProm memory
  if (pinCode != "MAST") 
  {
    Serial.println("Master code does NOT exist");
    pinCode = "MAST";
    for (int i = 0; i < 4; i++) 
    {  
      EEPROM.write(i,pinCode.charAt(i));
    }
    pinCode = "1234";     
    for (int i = 4; i < 8; i++) 
    {    
      EEPROM.write(i,pinCode.charAt(i-4));
    }
    // Since this is a new set we will reset the pinPoniter to '0'
    // and save this value to EEProm address '250'
    pinPointer = 0;
    EEPROM.write(pinPointerSave,pinPointer); 
    pinPointer = EEPROM.read(pinPointerSave);
    Serial.println("Initialized pinPointer = ");
    Serial.println(pinPointer);
    pinCode="";
  }
  else {
    //If this is not a new set then we will load the pinPointer into variable
    Serial.println("Master code DOES exist");
     //Get 4 characters of Master Pin
     pinCode="";
    for (int i = 4; i < 8; i++) {
    pinChar = (EEPROM.read(i));
    pinCode += pinChar;
  }
    pinPointer = EEPROM.read(pinPointerSave);
    Serial.print("Existing Master pinCode = ");
    Serial.println(pinCode);
    Serial.print("Existing pinPointer = ");
    Serial.println(pinPointer);
    pinCode="";
  }
  pinCode="";
}// End of initializeMaster function

// process a keypad number entry  ***********************************************
void processDigit(char keyIn)  
{
  if (pinCode.length() < 4)
    {
      pinCode += keyIn; //add digit to pinCode
      if(pinCode.length()==4) Serial.println("Pin Code entered is " + pinCode);
    }
    else status(4); //pinCode length ! <4
  
}//END processDigit

// playBeep***************************************************************************
void playBeep(int repeat, int duration){
  for (int i = 0; i < repeat; i++)
  {
    tone(piezoPin, frequency, duration);
    delay(duration * 1.30);
  }//END for
}//END playBeep()

// Process Timers**********************************************************************
void processTimers()
{
  unsigned long currentMillis = millis(); //get current timer
  if(currentMillis - previousMillis > interval)
    { 
      // save the last time you blinked the LED
      previousMillis = currentMillis;  
      interval = 500;
      // if the LED is off turn it on and vice-versa:
      if (ledState == LOW)
        ledState = HIGH;
      else
        ledState = LOW;
      // set the LED with the ledState of the variable:
      //Serial.println("Toggle LEDS");#######################
      toggleLedCode(ledState);
    } //End if(currentMillis - previousMillis > interval)

}//END processTimers()

//Toggele LEDS*******************************************************************
void toggleLedCode(int ledState)
{
  if (gT == 0 && rT == 0)
  {
    gT = gC;
    rT = rC;
  }

  if (gT >0)
  {
    digitalWrite(greenLed, ledState);
    if (ledState == LOW) gT--;
  }  //END if 

  else if (gT == 0 && rT > 0)
  {
    digitalWrite(redLed, ledState);
    if (ledState == LOW) rT--;
    if (ledState == LOW && rT == 0)  interval = 1000;
  }  //END else if

}  //END void toggleLedCode
