#include <SoftwareSerial.h> // Библиотека програмной реализации обмена по UART-протоколу 
#include <sms.h>

#define pir 2

SoftwareSerial SIM800(8, 9); // RX, TX 

SMSGSM smssd;

int pins[3] = {5, 6, 7}; // Пины с подключенными светодиодами 
String _response = ""; // Переменная для хранения ответа модуля 
long lastUpdate = millis(); // Время последнего обновления 
long updatePeriod = 3000; // Проверять каждые 3 секунды
String phones = "+79672910958"; // Белый список телефонов 
char num[14] = "+79672910958";
char Mot[20] = "Motion detected";
int stat[3] = {0,0,0}; 

void setup() 
{ 
  for (int i = 0; i < 3; i++) 
  { 
    pinMode(pins[i], OUTPUT); // Настраиваем пины в OUTPUT
    pinMode(pir, INPUT);
  } 
  Serial.begin(9600); // Скорость обмена данными с компьютером 
  SIM800.begin(9600); // Скорость обмена данными с модемом 
  Serial.println("Start!"); 
  sendATCommand("AT", true); // Отправили AT для настройки скорости обмена данными 
  sendATCommand("AT+CMGDA=\"DEL ALL\"", true);              
  // Удаляем все SMS, чтобы не забивать память 
  // Команды настройки модема при каждом запуске 
  //_response = sendATCommand("AT+CLIP=1", true);             
  // Включаем АОН 
  //_response = sendATCommand("AT+DDET=1", true);            
  // Включаем DTMF 
  sendATCommand("AT+CMGF=1;&W", true); // Включаем текстовый режима SMS (Text mode) и сразу сохраняем значение (AT&W)! 
  lastUpdate = millis(); // Обнуляем таймер 
}

 
String sendATCommand(String cmd, bool waiting)
{
  String _resp = ""; // Переменная для хранения результата
  Serial.println(cmd); // Дублируем команду в монитор порта
  SIM800.println(cmd); // Отправляем команду модулю
  if (waiting) // Если необходимо дождаться ответа...
  { 
    _resp = waitResponse(); // ... ждем, когда будет передан ответ 
    //Если Echo Mode выключен (ATE0), то эти 3 строки можно закомментировать 
    //if (_resp.startsWith(cmd)) // Убираем из ответа дублирующуюся команду
    //{ 
      //_resp = _resp.substring(_resp.indexOf("\r", cmd.length()) + 2);
    //} 
    Serial.println(_resp); // Дублируем ответ в монитор порта
  }
  return _resp; // Возвращаем результат. Пусто, если проблема
}


String waitResponse()  // Функция ожидания ответа и возврата полученного результата 
{
  String _resp = ""; // Переменная для хранения результата
  long _timeout = millis() + 2000; // Переменная для отслеживания таймаута (2 секунд)
  while (!SIM800.available() && millis() < _timeout) {}; // Ждем ответа 10 секунд, если пришел ответ или наступил таймаут, то...
  if (SIM800.available()) // Если есть, что считывать... 
  {
    _resp = SIM800.readString(); // ... считываем и запоминаем
  }
  else  // Если пришел таймаут, то...
  {
    Serial.println("Timeout..."); // ... оповещаем об этом и...
  }
  return _resp; // ... возвращаем результат. Пусто, если проблема
}


bool hasmsg = false; // Флаг наличия сообщений к удалению  ????????????????????????????????????????????


void parseSMS(String msg)  // Парсим SMS 
{
  String msgheader = "";
  String msgbody = "";
  String msgphone = "";
  msg = msg.substring(msg.indexOf("+CMGR: "));
  msgheader = msg.substring(0, msg.indexOf("\r")); // Выдергиваем телефон
  msgbody = msg.substring(msgheader.length() + 2);
  msgbody = msgbody.substring(0, msgbody.lastIndexOf("OK")); // Выдергиваем текст SMS
  msgbody.trim();
  int firstIndex = msgheader.indexOf("\",\"") + 3;
  int secondIndex = msgheader.indexOf("\",\"", firstIndex);
  msgphone = msgheader.substring(firstIndex, secondIndex);
  Serial.println("Phone: " + msgphone); // Выводим номер телефона
  Serial.println("Message: " + msgbody); // Выводим текст SMS
  if (msgphone.length() > 6 && phones.indexOf(msgphone) > -1)  // Если телефон в белом списке, то...
  {
    setLedState(msgbody, msgphone); // ...выполняем команду
  }
  else
  {
    Serial.println("Unknown phonenumber");
  }
}

       
void setLedState (String result, String phone) 
{
  bool correct = false; // Для оптимизации кода, переменная корректности команды
  String msgToSend = "";
  if (result.length() == 2)
  {
    int ledIndex = ((String)result[0]).toInt(); // Получаем первую цифру команды - адрес устройства (1-3)
    int ledState = ((String)result[1]).toInt(); // Получаем вторую цифру команды - состояние (0 - выкл, 1 - вкл)
    if (ledIndex >= 1 && ledIndex <= 3 && (ledState == 0 or ledState == 1)) 
    { // Если все нормально, исполняем команду
      msgToSend = "LED:" + (String)ledIndex + " set to " + (ledState == 0 ? "OFF" : "ON"); // Статус исполнения
      unsigned char* buf = new unsigned char[100];
      msgToSend.getBytes(buf, 100, 0);
      char *smsled = (char*)buf;
      if (gsm.begin(9600))
      {
        Serial.println("\nstatus=READY");
        smssd.SendSMS(num, smsled);
        SIM800.begin(9600); // Скорость обмена данными с модемом 
        sendATCommand("AT", true); // Отправили AT для настройки скорости обмена данными 
      }
      else
      {
        Serial.println("\nstatus=IDLE");
      }
      digitalWrite(pins[ledIndex - 1], ledState); // Исполняем команду
      stat[ledIndex-1]=ledState;
      correct = true; // Флаг корректности команды
    }
    if (ledIndex == 0 && ledState == 0)
    {
      msgToSend = "LED " + (String)1 + " is " + (stat[0] == 0 ? "OFF," : "ON,") + " LED " + (String)2 + " is " + (stat[1] == 0 ? "OFF," : "ON,") + " LED " + (String)3 + " is " + (stat[2] == 0 ? "OFF." : "ON."); // Статус исполнения
      unsigned char* buf = new unsigned char[100];
      msgToSend.getBytes(buf, 100, 0);
      char *smsled = (char*)buf;
      if (gsm.begin(9600))
      {
        Serial.println("\nstatus=READY");
        smssd.SendSMS(num, smsled);
        SIM800.begin(9600); // Скорость обмена данными с модемом 
        sendATCommand("AT", true); // Отправили AT для настройки скорости обмена данными 
      }
    }
  }
  //if (!correct)
  //{
    //msgToSend = "Incorrect command: " + result; // Статус исполнения
  //}
    //Serial.println(msgToSend); // Выводим результат в терминал
}
        
        
void sendSMS(String phone, String message)
{
  sendATCommand("AT+CMGS=\"" + phone + "\"", true); // Переходим в режим ввода текстового сообщения 
  sendATCommand(message + "\r\n" + (String)((char)26), true); // После текста отправляем перенос строки и Ctrl+Z
}


void loop() {
  if (lastUpdate + updatePeriod < millis() )  // Пора проверить наличие новых сообщений
  {
    do
    {
      int pirVal = digitalRead(pir);
      Serial.println(digitalRead(pir));
      if (pirVal)
      {
        Serial.println("Motion detected");
        //smssd.SendSMS(num, Mot);
      }
      _response = sendATCommand("AT+CMGL=\"REC UNREAD\",1", true);// Отправляем запрос чтения непрочитанных сообщений
      if (_response.indexOf("+CMGL: ") > -1) // Если есть хоть одно, получаем его индекс
      {
        int msgIndex = _response.substring(_response.indexOf("+CMGL: ") + 7, _response.indexOf("\"REC UNREAD\"", _response.indexOf("+CMGL: ")) - 1).toInt();
        char i = 0; // Объявляем счетчик попыток
        do 
        {
          i++; // Увеличиваем счетчик 
          _response = sendATCommand("AT+CMGR=" + (String)msgIndex + ",1", true); // Пробуем получить текст SMS по индексу 
          _response.trim(); // Убираем пробелы в начале/конце
          if (_response.endsWith("OK"))  // Если ответ заканчивается на "ОК"
          {
            if (!hasmsg) hasmsg = true; // Ставим флаг наличия сообщений для удаления
            sendATCommand("AT+CMGR=" + (String)msgIndex, true); // Делаем сообщение прочитанным
            sendATCommand("\n", true); // Перестраховка - вывод новой строки
            parseSMS(_response); // Отправляем текст сообщения на обработку
            break; // Выход из do{}
          }
          else  // Если сообщение не заканчивается на OK
          {
            Serial.println ("Error answer"); // Какая-то ошибка
            sendATCommand("\n", true); // Отправляем новую строку и повторяем попытку
          }
        } while (i < 10);
            break;
      }
      else 
      {
        lastUpdate = millis(); // Обнуляем таймер
        if (hasmsg) 
        {
          sendATCommand("AT+CMGDA=\"DEL ALL\"", true); // Удаляем все сообщения
          hasmsg = false;
        }
        break;
      }
    } while (1);
  }
  if (SIM800.available()) // Если модем, что-то отправил...
  {
    _response = waitResponse(); // Получаем ответ от модема для анализа 
    _response.trim(); // Убираем лишние пробелы в начале и конце
    Serial.println(_response); // Если нужно выводим в монитор порта
    if (_response.indexOf("+CMTI:")>-1)   // Пришло сообщение об отправке SMS
    {
      lastUpdate = millis() - updatePeriod; // Теперь нет необходимости обрабатываеть SMS здесь, достаточно просто // сбросить счетчик автопроверки и в следующем цикле все будет обработано
    }
  }
  if (Serial.available())  // Ожидаем команды по Serial... 
  {
    SIM800.write(Serial.read()); // ...и отправляем полученную команду модему
  }
}
