# 说明文档

此WebDav是来自[mongoose](https://github.com/cesanta/mongoose)的合并版本适配而来。

示例，如何移植到Arduino运行环境中。

``` c++
  // Because Arduino Have Spiffs module. and allow used in mongoose’s webdav， so you can init spiffs and inclue my mongoose.
  if (!SPIFFS.begin())
  {
      if (!SPIFFS.format())
          Serial.println("SPIFFS Format Failed");
      if (!SPIFFS.begin())
      {
          Serial.println("SPIFFS Mount Failed");
          return;
      }
  }
  // init mongoose and modity default root path to "/spiffs" in yours spiffs.
  // but i hope save work to remove auth for my webdav, so dont ask me about auth, my code not todo XD. 
  void mg_init()
  {
      // webdev
      opts.document_root = "/spiffs";
      opts.dav_document_root = "/spiffs";
      opts.dav_auth_file = "-";
      ...
      // etc.
      ...
  }
  //finally include my mongoose.c and simple used， know more? you can read the code find answer, thanks.
```

示例，如何移植到MicroPython运行环境中。

``` c++
  // i come in too.
  // micropython unlike arduino free modity code, that is not worth it.
  // You should know that the code should be in Rome as well.
  // so we to modity mpy(micropython) used fatfs to webdav, this not good idea, but it best fit for micropython.
  // such as arduino to init
  void mg_init()
  {
      // webdav
      opts.document_root = ".";
      opts.dav_document_root = ".";
      opts.dav_auth_file = "-";
      ...
      // etc.
      ...
  }
  // in you look, this webdav path not one, It depend on you.
  // But it's worth noting about C Define in mongoose.
  // such as #if CS_PLATFORM == CS_P_ESP32
  // this is used esp32. and in my code need more one define.
  // such asss #ifdef ESP32_FATFS
  // read code if you do not know how to do email me or ask for me. thx XD, please.
```
