volatile uint32_t *reg_timer_TIMERAWL = (volatile uint32_t *)0x40054028;
volatile uint32_t *reg_timer_TIMERAWH = (volatile uint32_t *)0x40054024;

volatile uint64_t timer_value = 0;
volatile uint64_t old_timer_value = 0;
volatile uint64_t elapse_time = 0;

volatile uint8_t acceleration_value = 1;
volatile bool acceleration_enable = true;

//////////////////////////////////// encoder ////////////////////////////////////
void encoder_running() {
  for (uint8_t enc=0; enc<sizeof(found_encoders); enc++) {
    if (found_encoders[enc] == false) continue;
    
    int32_t new_position= 0;
    if (enc<4)
    {
      new_position = encoders_1.getEncoderDelta(enc);      
    }
    else
    {
      new_position = encoders_2.getEncoderDelta(enc-4);
    }
    
    if (new_position!=0)
    { 
      if(!acceleration_enable) // no acceleration
      {     
        acceleration_value = 1;
      }
      else // with acceleration
      {   
        timer_value = *reg_timer_TIMERAWL + *reg_timer_TIMERAWH * 2^32; //read value of timer counter
        elapse_time = timer_value - old_timer_value; //time since last interruption
        if(elapse_time>500000){acceleration_value = 0;} //if slow --> no acceleration
        else{acceleration_value = 127*exp(-0.000022*elapse_time); }  //if fast --> acceleration
        old_timer_value = timer_value;
      }

      if(!digitalRead(B1_READ)){ //midi nav      
        movement = new_position;
        midi_channel = read_midi_value_encoder(enc);
        if(midi_channel + movement >=0 & midi_channel + movement<16)
        {
          midi_channel = midi_channel + movement;
          set_midi_value_encoder(enc, midi_channel);

          CC_number = read_CC_number_value_encoder(enc);        
          CC_value = get_value_encoder(enc);
          refresh = true;
        }
      }
      else if(!digitalRead(B2_READ)){ //CC_number nav
        movement = new_position*(1+uint64_t(acceleration_value));
        CC_number = read_CC_number_value_encoder(enc);
        
        if(!(CC_number==0 & movement<0) || (CC_number==127 & movement>0))
        {
          if(CC_number+movement>127){CC_number=127;}
          else if (CC_number+movement<0){CC_number=0;}
          else{CC_number+=movement;}

          set_CC_number_value_encoder(enc, CC_number);

          midi_channel = read_midi_value_encoder(enc);
          CC_value = get_value_encoder(enc);
          refresh = true;
        }
      }
      else{ //CC_value nav
        movement = new_position*(1+uint64_t(acceleration_value));
        CC_value = get_value_encoder(enc);
        if(!(CC_value==0 & movement<0) || (CC_value==127 & movement>0))
        {
          if(CC_value+movement>127){CC_value=127;}
          else if (CC_value+movement<0){CC_value=0;}
          else{CC_value+=movement;}
     
          write_value_data(midi_channel, CC_number, CC_value);

          CC_number = read_CC_number_value_encoder(enc);
          midi_channel = read_midi_value_encoder(enc);
          refresh = true;
          send_midi(midi_channel, CC_number, CC_value);
        }
      }
      
      if (refresh)
      {
        refresh = false;
        refresh_screen(midi_channel, CC_number, CC_value);
      }
    }
  }

  delay(10);
}

void send_midi(uint8_t midi_channel, uint8_t CC_number, uint8_t CC_value){
  //Serial1.write(0b1011 << 4 | midi_channel);
  Serial1.write(176+midi_channel);
  Serial1.write(CC_number);
  Serial1.write(CC_value);
}

//if the 3 buttons are pushed, we reset all the value stored in the SD
bool reset_table(){
  if (!digitalRead(B1_READ) && !digitalRead(B2_READ) && (
    !encoders_1.digitalRead(SS_ENC0_SWITCH) || !encoders_1.digitalRead(SS_ENC1_SWITCH) || !encoders_1.digitalRead(SS_ENC2_SWITCH) || !encoders_1.digitalRead(SS_ENC3_SWITCH) ||
    !encoders_2.digitalRead(SS_ENC0_SWITCH) || !encoders_2.digitalRead(SS_ENC1_SWITCH) || !encoders_2.digitalRead(SS_ENC2_SWITCH) || !encoders_2.digitalRead(SS_ENC3_SWITCH) 
    )) {
    clear_table_data();
    clear_table_encoder();
    refresh_screen(0, 0, 0);  
  
    delay(1000);
    return true;
  }
  return false;
}