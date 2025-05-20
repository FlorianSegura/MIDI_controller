//helper high level
void clear_table_data() {
  midi_table.seek(0);
  uint8_t zero = 0;
  for (int i_midi=0; i_midi<16; i_midi++){
    for (int i_CC_number=0; i_CC_number<128; i_CC_number++){
      midi_table.write(zero);
    }
  }
  midi_table.flush();  
}

void clear_table_encoder() {
  encoder_table.seek(0);
  uint8_t zero = 0;
  for (int i_encoder=0; i_encoder<NB_ENCODER; i_encoder++){
    encoder_table.write(zero); //midi
    encoder_table.write(zero); //CC-number
  }
  encoder_table.flush();  
}

uint8_t get_value_encoder(uint8_t encoder) {  
  read_value_data(read_midi_value_encoder(encoder), read_CC_number_value_encoder(encoder));
  return midi_table.peek();
}

void set_value_encoder(uint8_t encoder, uint8_t value) {  
  write_value_data(read_midi_value_encoder(encoder), read_CC_number_value_encoder(encoder), value);
}

void set_midi_value_encoder(uint8_t encoder, uint8_t midi) {
  encoder_table.seek(encoder*2);
  encoder_table.write(midi);
  encoder_table.flush();
}

void set_CC_number_value_encoder(uint8_t encoder, uint8_t CC_number) {
  encoder_table.seek(encoder*2 + 1);
  encoder_table.write(CC_number);
  encoder_table.flush();
}

//helper low level
uint8_t read_midi_value_encoder(uint8_t encoder) {
  encoder_table.seek(encoder*2);
  return encoder_table.peek();
}

uint8_t read_CC_number_value_encoder(uint8_t encoder) {
  encoder_table.seek(encoder*2 + 1);
  return encoder_table.peek();
}

uint8_t read_value_data(uint8_t midi, uint8_t CC_number) {
  midi_table.seek(midi*128 + CC_number);
  return midi_table.peek();
}

void write_value_data(uint8_t midi, uint8_t CC_number, uint8_t CC_value){
  midi_table.seek(midi*128 + CC_number);
  midi_table.write(CC_value);
  midi_table.flush();
} 