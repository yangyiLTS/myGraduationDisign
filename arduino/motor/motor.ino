int Pin0 = 8;
int Pin1 = 9;
int Pin2 = 10;
int Pin3 = 11;
int _step = 0 ;
boolean dir; //= -1;正反转
int stepperSpeed = 1;//电机转速,1ms一步
void setup()
{
  pinMode(Pin0, OUTPUT);
  pinMode(Pin1, OUTPUT);
  pinMode(Pin2, OUTPUT);
  pinMode(Pin3, OUTPUT);
  pinMode(4, INPUT);
}

bool step[4][4] = {{1, 0, 0, 1}, {1, 0, 1, 0}, {0, 1, 1, 0}, {0, 1, 0, 1}};


void loop()
{
  down();
  delay(100);
}


void down()
{
  for (int i = 0; i < 4; i++) {
    digitalWrite(Pin0, step[i][0]);
    digitalWrite(Pin1, step[i][1]);
    digitalWrite(Pin2, step[i][2]);
    digitalWrite(Pin3, step[i][3]);
    delay(stepperSpeed);
  }
  digitalWrite(Pin0, 0);
  digitalWrite(Pin1, 0);
  digitalWrite(Pin2, 0);
  digitalWrite(Pin3, 0);
}



void up()
{

  switch (_step) {
    case 0:
      //stepperSpeed++;
      digitalWrite(Pin0, 0);
      digitalWrite(Pin1, 1);
      digitalWrite(Pin2, 0);
      digitalWrite(Pin3, 1);//32A
      break;
    case 1:
      digitalWrite(Pin0, 0);
      digitalWrite(Pin1, 1);//10B
      digitalWrite(Pin2, 1);
      digitalWrite(Pin3, 0);
      break;
    case 2:
      digitalWrite(Pin0, 1);
      digitalWrite(Pin1, 0);
      digitalWrite(Pin2, 1);
      digitalWrite(Pin3, 0);
      break;
    case 3:
      digitalWrite(Pin0, 1);
      digitalWrite(Pin1, 0);
      digitalWrite(Pin2, 0);
      digitalWrite(Pin3, 1);
      break;
  }
  _step++;

  if (_step > 3) {
    _step = 0;
  }
  delay(stepperSpeed);
}

