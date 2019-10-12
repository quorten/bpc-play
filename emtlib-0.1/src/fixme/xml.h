//XML core OOP data module
//XmlFile is an abstract class, meant to be derived to a class which handles the data creation routines.
//Classes XmlElement and XmlAttribute are meant to be derived to classes with actual data
//storage.

#ifndef XML_H
#define XML_H

#include <map>
#include <string>
#include <list>

//Forword Declarations
class XmlElement;
class XmlAttribute;

//Enumerants
enum ParseDirection {Encode, Decode};

//Class Declarations
class XmlFile
{
protected:
	std::string originalData; //Temporary storage
	char* originalFilename;
	bool modified;
	bool ParseData(ParseDirection parseDirection); //Translate data to text or not
	XmlElement* CreateElement(std::string name, XmlElement* parent) = 0;
	bool AddAttribute(std::string attName, std::string attValue, XmlElement* owner) = 0;
public:
	XmlFile(const std::string filename = NULL, bool keepOriginalData = false);
	~XmlFile();
	bool Load(const std::string filename, bool keepOriginalData = false);
	bool Unload();
	bool Save(const std::string filename = NULL);
	void Revert();
	XmlElement* dtd;
	XmlElement* root;
};

class XmlElement
{
public:
	void clear() = 0;
};

class XmlAttribute
{
};

//Dynamic data modules. Used for unknown infosets, or robust XML DOM
template <class prefType = std::string> class XmlElement_Dyn : public XmlElement //-How to optionally represent pcData-
{
public:
	XmlElement_Dyn(const std::string name);
	~XmlElement_Dyn();
	void clear();
	unsigned elementName; //element name ID
	std::map<unsigned, XmlElement> subElements;
	std::list<XmlAttribute> attributes;
	std::map<unsigned, prefType> pcData;
};

template <class prefType = std::string> class XmlAttribute_Dyn : public XmlAttribute
{
public:
	unsigned attName; //attribute name ID
	prefType attValue;
};

#endif
