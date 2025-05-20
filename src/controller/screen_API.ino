//display midi channel on the top left corner
void display_midi_channel(uint8_t value){
  display.setTextSize(2);
  display.setCursor(0, 0); 
  if (value >= 0 && value < 16)  {    display.println(value);  }
  else{    display.println("error");  }
}

//display CC number on the top right corner
void display_CC_number(uint8_t value){
  
  display.setTextSize(2);
  
  if (value >= 0 && value < 10)  {    display.setCursor(116, 0);  }
  else if (value >= 10 && value < 100)  {    display.setCursor(104, 0);  }
  else if (value >= 100 && value < 128)  {    display.setCursor(92, 0);  }
  else
  {
    display.setCursor(68, 0);
    display.println("error");
    return;
  }

  display.println(value);
}

//display CC value on the center of the screen
void display_CC_value(uint8_t value){
  
  display.setTextSize(3);

  if (value >= 0 && value < 10)  {    display.setCursor(56, 30);  }
  else if (value >= 10 && value < 100)  {    display.setCursor(47, 30);  }
  else if (value >= 100 && value < 128)  {    display.setCursor(34, 30);  }
  else
  {
    display.setCursor(20, 30);
    display.println("error");
    return;
  }

  display.println(value);
}

//fonction to update the screen
void refresh_screen(uint8_t i_midi_channel, uint8_t i_cc_number, uint8_t i_cc_value)
{
  display.clearDisplay();
  display_midi_channel(i_midi_channel);
  display_CC_number(i_cc_number);
  display_CC_value(i_cc_value);
  display.display();
}