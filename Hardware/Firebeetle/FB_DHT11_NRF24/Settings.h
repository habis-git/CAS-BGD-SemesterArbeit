void readLine( const char* str );
void setSettingsValue(String argument, String Wert);

boolean readSettings()
{
  String      line ;
  File        settingsfile ;

  settingsfile = SPIFFS.open ( "/settings.txt", "r" ) ;
  Serial.println("");
  Serial.println("Read settings.txt file");
  while (settingsfile.available())
  {
    line = settingsfile.readStringUntil ( '\n' ) ;
    readLine ( line.c_str() ) ;
  }
  settingsfile.close() ;
  return true;
}

void readLine ( const char* str )
{
  char*  value ;                                 // Points to value after equalsign in command

  value = strstr ( str, "=" ) ;                  // See if command contains a "="
  if ( value )
  {
    *value = '\0' ;                              // Separate command from value
    value++ ;                                    // Points to value after "="
  }
  else
  {
    value = (char*) "0" ;                        // No value, assume zero
  }


  String             argument ;                       // Argument as string
  String             Wert ;                          // Value of an argument as a string
  int                inx ;                            // Index in string

  argument = str;
  if ( (inx = argument.indexOf ( "#" ) ) >= 0 )           // Comment line or partial comment?
  {
    argument.remove ( inx ) ;                              // Yes, remove
  }
  argument.trim() ;                                        // Remove spaces and CR
  if ( argument.length() == 0 )                       // Lege commandline (comment)?
  {
    return ;
  }
  argument.toLowerCase() ;                            // Force to lower case
  Wert = value ;
  if ( (inx = Wert.indexOf ( "#" ) ) >= 0 )           // Comment line or partial comment?
  {
    Wert.remove ( inx ) ;                              // Yes, remove
  }
  Wert.trim() ;                                        // Remove spaces and CR
  setSettingsValue(argument, Wert);
}
