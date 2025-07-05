#include <Servo.h>
#include <DHT11.h>

float triangle_mf(float val, float start, float top, float end){
  float side_a = (val-start)/(top-start);
  float side_b = (end-val)/(end-top);

  return max(min(side_a, side_b), 0);
}

float trapeze_mf(float val, float start, float top_1, float top_2, float end){
  float side_a = (val-start)/(top_1-start);
  float side_b = (end-val)/(end-top_2);

  return max(min(1, min(side_a, side_b)),0);
}

float gauss_mf(float val, float st_dev, float center){
  float e = 2.71828;

  return pow(e, -pow(val - center, 2) / (2  * pow(st_dev, 2)));
}

float min_arr(float numbers[]){
  float minimum = 100;

  for (int i = 0; i <= sizeof(numbers); i++){
    if (numbers[i] < minimum && numbers[i] >= 0){
      minimum = numbers[i];
    }
  }
  return minimum;
}

float max_arr(float numbers[]){
  float maximum = 0;

  for (int i = 0; i < sizeof(numbers); i++){
    if (numbers[i] > maximum){
      maximum = numbers[i];
    }
  }
  return maximum;
}

Servo valve;
DHT11 dht11(7);

int temp;
int dht_hum;
int hum;
int rad;

void setup() {
  Serial.begin(9600);

  pinMode(3, OUTPUT);

  valve.attach(8);
  valve.write(0);

  pinMode(6, OUTPUT);
}

void loop() {
  //Readings
  rad = analogRead(0);
  hum = analogRead(1);
  int dht_output = dht11.readTemperatureHumidity(temp, dht_hum);

  //Serial monitoring
  Serial.print("Humedad: ");
  Serial.println(hum);
  Serial.print("Radiacion solar: ");
  Serial.println(rad);
  Serial.print("Temperatura: ");
  Serial.println(temp);
  Serial.println("");

  //Humidity mf
  float h_wet = triangle_mf(hum, -1, 0, 250);
  float h_med = triangle_mf(hum, 100, 450, 700);
  float h_dry = triangle_mf(hum, 600, 1023, 1024);

  //Temperature mf
  float t_cold = gauss_mf(temp, 10, 0);
  float t_med = gauss_mf(temp, 4, 23);
  float t_hot = gauss_mf(temp, 12, 50);

  //Radiation mf
  float r_low = trapeze_mf(rad, 700, 900, 1023, 1024);
  float r_med = trapeze_mf(rad, 300, 500, 700, 900);
  float r_high = trapeze_mf(rad, -1, 0, 300, 500);

  //Output mf
  float i_low = 30;
  float i_med = 60;
  float i_high = 100;

  float c_off = 0;
  float c_med = 50;
  float c_high = 100;

  //Rules definition
  float rules[20][3] = {
    {h_wet, 1, 1},
    {t_hot, 1, 1},
    {h_med, t_cold, 1},
    {h_dry, t_cold, r_low},
    {h_dry, t_cold, r_med},
    {h_dry, r_high, 1},
    {t_cold, r_med, 1},
    {t_cold, r_low, 1},
    {h_dry, t_med, 1},
    {h_med, t_med, 1},
    {h_med, t_med, r_low},
    {h_wet, t_med, r_low},
    {t_med, r_high, 1},
    {h_dry, t_hot, 1},
    {h_med, t_hot, r_low},
    {h_med, t_med, r_med},
    {h_med, t_hot, r_high},
    {h_med, t_cold, r_high},
    {h_wet, t_cold, r_high}
    };


  //Irrigation
  float i_low_rules[7] = {
    min_arr(rules[0]),
    min_arr(rules[2]),
    min_arr(rules[3]),
    min_arr(rules[7]),
    min_arr(rules[11]),
    min_arr(rules[18]),
    min_arr(rules[19])
  };

  float i_med_rules[4] = {
    min_arr(rules[4]),
    min_arr(rules[9]),
    min_arr(rules[10]),
    min_arr(rules[15]),
  };

  float i_high_rules[5] = {
    min_arr(rules[5]),
    min_arr(rules[8]),
    min_arr(rules[14]),
    min_arr(rules[16]),
    min_arr(rules[17]),
  };

  //Calefaction
  float c_off_rules[9] = {
    min_arr(rules[1]),
    min_arr(rules[5]),
    min_arr(rules[8]),
    min_arr(rules[12]),
    min_arr(rules[13]),
    min_arr(rules[14]),
    min_arr(rules[15]),
    min_arr(rules[16]),
    min_arr(rules[17])
  };

  float c_med_rules[3] = {
    min_arr(rules[10]),
    min_arr(rules[11]),
    min_arr(rules[18])
  };

  float c_high_rules[5] = {
    min_arr(rules[3]),
    min_arr(rules[4]),
    min_arr(rules[6]),
    min_arr(rules[7]),
    min_arr(rules[19])
  };

  //Aggregations
  float i_agg[3] = {
    max_arr(i_low_rules),
    max_arr(i_med_rules),
    max_arr(i_high_rules)
  };

  float c_agg[3] = {
    max_arr(c_off_rules),
    max_arr(c_med_rules),
    max_arr(c_high_rules)
  };

  //Defuzzyfied totals
  float i_total = ((i_low * i_agg[0]) + (i_med * i_agg[1]) + (i_high * i_agg[2])) / (i_agg[0] + i_agg[1] + i_agg[2]);
  float c_total = ((c_off * c_agg[0]) + (c_med * c_agg[1]) + (c_high * c_agg[2])) / (c_agg[0] + c_agg[1] + c_agg[2]);

  //Output in serial
  Serial.print("Riego total: ");
  Serial.println(i_total);
  Serial.print("Calefaccion total: ");
  Serial.println(c_total);
  Serial.println("\n");

  //Output in actuators
  valve.write(i_total);
  analogWrite(6, map(c_total, 0, 100, 0, 255));
}
