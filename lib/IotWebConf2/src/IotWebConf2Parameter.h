/**
 * IotWebConf2Parameter.h -- IotWebConf2 is an ESP8266/ESP32
 *   non blocking WiFi/AP web configuration library for Arduino.
 *   https://github.com/prampec/IotWebConf2
 *
 * Copyright (C) 2020 Balazs Kelemen <prampec+arduino@gmail.com>
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef IotWebConf2Parameter_h
#define IotWebConf2Parameter_h

#include <Arduino.h>
#include <functional>
#include <IotWebConf2Settings.h>
#include <IotWebConf2WebServerWrapper.h>

const char IOTWEBCONF_HTML_FORM_GROUP_START[] PROGMEM =
  "<fieldset id='{i}'><legend>{b}</legend>\n";
const char IOTWEBCONF_HTML_FORM_GROUP_END[] PROGMEM =
  "</fieldset>\n";

const char IOTWEBCONF_HTML_FORM_PARAM[] PROGMEM =
  "<div class='{s}' style='margin:0.6rem 0;'><label for='{i}'>{b}</label><input type='{t}' id='{i}' "
  "name='{i}' {l} placeholder='{p}' value='{v}' {c}/>"
  "<div class='em'>{e}</div></div>\n";

const char IOTWEBCONF_HTML_FORM_SELECT_PARAM[] PROGMEM =
  "<div class='{s}' style='margin:0.6rem 0;'><label for='{i}'>{b}</label><select id='{i}' "
  "name='{i}' {c}/>\n{o}"
  "</select><div class='em'>{e}</div></div>\n";
const char IOTWEBCONF_HTML_FORM_OPTION[] PROGMEM =
  "<option value='{v}'{s}>{n}</option>\n";

namespace iotwebconf2
{

typedef struct SerializationData
{
  byte* data;
  int length;
} SerializationData;

class ConfigItem
{
public:
  bool visible = true;
  const char* getId() { return this->_id; }

  /**
   * Calculate the size of bytes should be stored in the EEPROM.
   */
  virtual int getStorageSize() = 0;

  /**
   * On initial startup (when no data was saved), it may be required to apply a default value
   *   to the parameter.
   */
  virtual void applyDefaultValue() = 0;

  /**
   * Save data.
   * @doStore - A method is passed as a parameter, that will performs the actual EEPROM access.
   *   The argument 'serializationData' of this referenced method should be pre-filled with
   *   the size and the serialized data before calling the method.
   */
  virtual void storeValue(std::function<void(SerializationData* serializationData)> doStore) = 0;

  /**
   * Load data.
   * @doLoad - A method is passed as a parameter, that will performs the actual EEPROM access.
   *   The argument 'serializationData' of this referenced method should be pre-filled with
   *   the size of the expected data, and the data buffer should be allocated with this size.
   *   The doLoad will fill the data from the EEPROM.
   */
  virtual void loadValue(std::function<void(SerializationData* serializationData)> doLoad) = 0;

  /**
   * This method will create the HTML form item for the config portal.
   * 
   * @dataArrived - True if there was a form post, where (some) form
   *   data arrived from the client.
   * @webRequestWrapper - The webRequestWrapper, that will send the rendered content to the client.
   *   The webRequestWrapper->sendContent() method should be used in the implementations.
   */
  virtual void renderHtml(bool dataArrived, WebRequestWrapper* webRequestWrapper) = 0;

  /**
   * New value arrived from the form post. The value should be stored in the
   *   in this config item.
   * 
   * @webRequestWrapper - The webRequestWrapper, that will send the rendered content to the client.
   *   The webRequestWrapper->hasArg() and webRequestWrapper->arg() methods should be used in the
   *   implementations.
   */
  virtual void update(WebRequestWrapper* webRequestWrapper) = 0;

  /**
   * Before validating the form post, it is required to clear previos error messages.
   */
  virtual void clearErrorMessage() = 0;

  /**
   * This method should display information to Serial containing the parameter
   *   ID and the current value of the parameter (if it is confidential).
   *   Will only be called if debug is enabled.
   */
  virtual void debugTo(Stream* out) = 0;

protected:
  ConfigItem(const char* id) { this->_id = id; };

private:
  const char* _id = 0;
  ConfigItem* _parentItem = NULL;
  ConfigItem* _nextItem = NULL;
  friend class ParameterGroup; // Allow ParameterGroup to access _nextItem.
};

class ParameterGroup : public ConfigItem
{
public:
  ParameterGroup(const char* id, const char* label = NULL);
  void addItem(ConfigItem* configItem);
  const char *label;
  void applyDefaultValue() override;

protected:
  int getStorageSize() override;
  void storeValue(std::function<void(
    SerializationData* serializationData)> doStore) override;
  void loadValue(std::function<void(
    SerializationData* serializationData)> doLoad) override;
  void renderHtml(bool dataArrived, WebRequestWrapper* webRequestWrapper) override;
  void update(WebRequestWrapper* webRequestWrapper) override;
  void clearErrorMessage() override;
  void debugTo(Stream* out) override;
  /**
   * One can override this method in case a specific HTML template is required
   * for a group.
   */
  virtual String getStartTemplate() { return FPSTR(IOTWEBCONF_HTML_FORM_GROUP_START); };
  /**
   * One can override this method in case a specific HTML template is required
   * for a group.
   */
  virtual String getEndTemplate() { return FPSTR(IOTWEBCONF_HTML_FORM_GROUP_END); };

  ConfigItem* _firstItem = NULL;
  ConfigItem* getNextItemOf(ConfigItem* parent) { return parent->_nextItem; };

  friend class IotWebConf2; // Allow IotWebConf2 to access protected members.

private:
};

/**
 * Parameters is a configuration item of the config portal.
 * The parameter will have its input field on the configuration page,
 * and the provided value will be saved to the EEPROM.
 */
class Parameter : public ConfigItem
{
public:
  /**
   * Create a parameter for the config portal.
   *
   * @label - Displayable label at the config portal.
   * @id - Identifier used for HTTP queries and as configuration key. Must not
   *   contain spaces nor other special characters.
   * @valueBuffer - Configuration value will be loaded to this buffer from the
   *   EEPROM.
   * @length - The buffer should have a length provided here.
   * @defaultValue - Defalt value set on startup, when no configuration ever saved
   *   with the current config-version.
   */
  Parameter(
    const char* label, const char* id, char* valueBuffer, int length,
    const char* defaultValue = NULL);

  const char* label;
  char* valueBuffer;
  const char* defaultValue;
  const char* errorMessage;

  int getLength() { return this->_length; }
  void applyDefaultValue() override;

protected:
  // Overrides
  int getStorageSize() override;
  void storeValue(std::function<void(SerializationData* serializationData)> doStore) override;
  void loadValue(std::function<void(SerializationData* serializationData)> doLoad) override;
  virtual void update(WebRequestWrapper* webRequestWrapper) override;
  virtual void update(String newValue) = 0;
  void clearErrorMessage() override;

private:
  int _length;
};

///////////////////////////////////////////////////////////////////////////////

/**
 * TexParameters is to store text based parameters.
 */
class TextParameter : public Parameter
{
public:
  /**
   * Create a text parameter for the config portal.
   *
   * @placeholder (optional) - Text appear in an empty input box.
   * @customHtml (optional) - The text of this parameter will be added into
   *   the HTML INPUT field.
   * (See Parameter for arguments!)
   */
  TextParameter(
    const char* label, const char* id, char* valueBuffer, int length,
    const char* defaultValue = NULL,
    const char* placeholder = NULL,
    const char* customHtml = NULL);

  /**
   * This variable is meant to store a value that is displayed in an empty
   *   (not filled) field.
   */
  const char* placeholder;

  /**
   * Usually this variable is used when rendering the form input field
   *   so one can customize the rendered outcome of this particular item.
   */
  const char* customHtml;

protected:
  virtual String renderHtml(
    bool dataArrived, bool hasValueFromPost, String valueFromPost);
  // Overrides
  virtual void renderHtml(bool dataArrived, WebRequestWrapper* webRequestWrapper) override;
  virtual void update(String newValue) override;
  virtual void debugTo(Stream* out) override;
  /**
   * One can override this method in case a specific HTML template is required
   * for a parameter.
   */
  virtual String getHtmlTemplate() { return FPSTR(IOTWEBCONF_HTML_FORM_PARAM); };

  /**
   * Renders a standard HTML form INPUT.
   * @type - The type attribute of the html input field.
   */
  virtual String renderHtml(const char* type, bool hasValueFromPost, String valueFromPost);

private:
  friend class IotWebConf2;
  friend class WifiParameterGroup;
};

///////////////////////////////////////////////////////////////////////////////

/**
 * The Password parameter has a special handling, as the value will be
 * overwritten in the EEPROM only if value was provided on the config portal.
 * Because of this logic, "password" type field with length more then
 * IOTWEBCONF_PASSWORD_LEN characters are not supported.
 */
class PasswordParameter : public TextParameter
{
public:
  /**
   * Create a password parameter for the config portal.
   *
   * (See TextParameter for arguments!)
   */
  PasswordParameter(
    const char* label, const char* id, char* valueBuffer, int length,
    const char* defaultValue = NULL,
    const char* placeholder = NULL,
    const char* customHtml = "ondblclick=\"pw(this.id)\"");

protected:
  // Overrides
  virtual String renderHtml(
    bool dataArrived, bool hasValueFromPost, String valueFromPost) override;
  virtual void update(String newValue) override;
  virtual void debugTo(Stream* out) override;

private:
  friend class IotWebConf2;
  friend class WifiParameterGroup;
};

///////////////////////////////////////////////////////////////////////////////

/**
 * This is just a text parameter, that is rendered with type 'number'.
 */
class NumberParameter : public TextParameter
{
public:
  /**
   * Create a numeric parameter for the config portal.
   *
   * (See TextParameter for arguments!)
   */
  NumberParameter(
    const char* label, const char* id, char* valueBuffer, int length,
    const char* defaultValue = NULL,
    const char* placeholder = NULL,
    const char* customHtml = NULL);

protected:
  // Overrides
  virtual String renderHtml(
    bool dataArrived, bool hasValueFromPost, String valueFromPost) override;

private:
  friend class IotWebConf2;
};

///////////////////////////////////////////////////////////////////////////////

/**
 * Checkbox parameter is represended as a text parameter but has a special
 * handling. As the value is either empty or has the word "selected".
 * Note, that form post will not send value if checkbox was not selected.
 */
class CheckboxParameter : public TextParameter
{
public:
  /**
   * Create a checkbox parameter for the config portal.
   *
   * (See TextParameter for arguments!)
   */
  CheckboxParameter(
    const char* label, const char* id, char* valueBuffer, int length,
    bool defaultValue = false);
  bool isChecked() { return strncmp(this->valueBuffer, "selected", this->getLength()) == 0; }

protected:
  // Overrides
  virtual String renderHtml(
    bool dataArrived, bool hasValueFromPost, String valueFromPost) override;
  virtual void update(WebRequestWrapper* webRequestWrapper) override;

private:
  friend class IotWebConf2;
  bool _checked;
  const char* _checkedStr = "checked='checked'";
};

///////////////////////////////////////////////////////////////////////////////

/**
 * Options parameter is a structure, that handles multiple values when redering
 * the HTML representation.
 */
class OptionsParameter : public TextParameter
{
public:
  /**
   * @optionValues - List of values to choose from with, where each value
   *   can have a maximal size of 'length'. Contains 'optionCount' items.
   * @optionNames - List of names to render for the values, where each
   *   name can have a maximal size of 'nameLength'. Contains 'optionCount'
   *   items.
   * @optionCount - Size of both 'optionValues' and 'optionNames' lists.
   * @nameLength - Size of any item in optionNames list.
   * (See TextParameter for arguments!)
   */
  OptionsParameter(
    const char* label, const char* id, char* valueBuffer, int length,
    const char* optionValues, const char* optionNames, size_t optionCount, size_t nameLength,
    const char* defaultValue = NULL);

protected:
  const char* _optionValues;
  const char* _optionNames;
  size_t _optionCount;
  size_t _nameLength;

private:
  friend class IotWebConf2;
};

///////////////////////////////////////////////////////////////////////////////

/**
 * Select parameter is an option parameter, that rendered as HTML SELECT.
 * Basically it is a dropdown combobox.
 */
class SelectParameter : public OptionsParameter
{
public:
  /**
   * Create a select parameter for the config portal.
   *
   * (See OptionsParameter for arguments!)
   */
  SelectParameter(
    const char* label, const char* id, char* valueBuffer, int length,
    const char* optionValues, const char* optionNames, size_t optionCount, size_t namesLenth,
    const char* defaultValue = NULL);

protected:
  // Overrides
  virtual String renderHtml(
    bool dataArrived, bool hasValueFromPost, String valueFromPost) override;

private:
  friend class IotWebConf2;
};

/**
 * This class is here just to make some nice indents on debug output
 *   for group tree.
 */
class PrefixStreamWrapper : public Stream
{
public:
  PrefixStreamWrapper(
    Stream* originalStream,
    std::function<size_t(Stream* stream)> prefixWriter);
  size_t write(uint8_t) override;
  size_t write(const uint8_t *buffer, size_t size) override;
  int available() override { return _originalStream->available(); };
  int read() override { return _originalStream->read(); };
  int peek() override { return _originalStream->peek(); };
  void flush() override { return _originalStream->flush(); };

private:
  Stream* _originalStream;
  std::function<size_t(Stream* stream)> _prefixWriter;
  bool _newLine = true;

  size_t checkNewLine();
};

} // end namespace

#endif
