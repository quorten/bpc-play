//XML core OOP data module

#include "xml.h"
#include <fstream>
#include <stack>

using namespace std;

XmlFile::XmlFile(const string filename, bool keepOriginalData)
{
	if (filename.empty() == false)
		Load(filename, keepOriginalData);
}

XmlFile::~XmlFile()
{
	Unload();
}

bool XmlFile::Load(const string filename, bool keepOriginalData)
{
	originalFilename = new char[strlen(filename)+1];
	strcpy(originalFilename, filename);
	originalFilename[strlen(filename)] = '\0';
	ifstream file(filename.c_str());
	if (file.fail())
		return false;

	file.read(originalData);
	file.close();
	return ParseData(false);
}

bool XmlFile::ParseData(ParseDirection parseDirection)
{
	string::iterator iter;
	stack<string> elementStack;
	stack<XmlElement*> elementDataStack;
	//All of the boolean variables below are used for reporting markup violations
	bool inProcInst = false; //Processing Instruction
	bool inElement = false;
	bool inTag = false;
	bool inAttribute = false;
	bool inQuotes = false;
	for (iter = originalData.begin(); iter != originalData.end(); iter++)
	{
		if (*iter == '<')
		{
			iter++;
			if (*iter == '?') //processing instruction
			{
				//We don't support PI's right now (except for <?xml...?>
				iter++;
				if (*iter == 'x' && *(iter+1) == 'm' && *(iter+2) == 'l')
				{
					iter += 3;
					for (; *iter != '>' && iter != originalData.end(); iter++)
					{
						if (*iter != '\t' && *iter != '\n' && *iter != ' ')
						{
							if (strncmp(*iter, "version", 7)
							{
							}
						}
					}
				}
				else //No other processing instructions accepted
					for (; *iter != '>' && iter != originalData.end(); iter++);
				
			}
			else if (*iter == '/') //closing tag
			{
				iter++;
				string temp(1, *iter);
				for (; *iter != '>' && iter != originalData.end(); iter++)
				{
					temp.append(1, *iter);
				}
				if (elementStack.top() == temp)
				{
					elementStack.pop();
					elementDataStack.pop();
				}
				else
					return false;
			}
			else if (*iter == '!') //!DOCTYPE or comment
			{
				iter++;
				if (*iter == '-' && *(iter+1) == '-')
				{
					iter += 2;
					for (; *(iter-2) != '-' && *(iter-1) != '-' && *iter != '>' &&  iter != originalData.end(); iter++);
				}
				else
				{
					string temp(1, *iter);
					for (; *iter != '\t' && *iter != '\n' && *iter != ' ' && iter != originalData.end(); iter++)
						temp.append(1, *iter);
				}
			}
			else //opening tag or empty element
			{
				elementStack.push(string(1, *iter));
				for (; *iter != '\t' &&
					*iter != '\n' &&
					*iter != ' ' &&
					*iter != '>' &&
					*iter != '/' &&
					iter != originalData.end(); iter++)
				{
					elementStack.top().append(1, *iter);
				}
				if (elementDataStack.size() == 0)
				{
					if (root != NULL)
						return false;
					root = CreateElement(elementStack.top(), NULL);
					elementDataStack.push(root);
				}
				else
				{
					elementDataStack.push(CreateElement(elementStack.top(), elementDataStack.top()));
				}
				if (*iter != '>' && *iter != '/')
				{
					//attributes
					for (; *iter == '\t' || *iter == '\n' || *iter == ' ' && iter != originalData.end(); iter++);
					for (; *iter != '>' && *iter != '/' && iter != originalData.end(); iter++)
					{
						string name(1, *iter);
						for (; *iter != '=' && iter != originalData.end(); iter++)
						{
							name.append(1, *iter);
						}
						iter++;
						string value;
						if (*iter == '"')
						{
							for (; *iter != '"' && iter != originalData.end(); iter++)
								value.append(1, *iter);
						}
						else if (*iter == '\'')
						{
							for (; *iter != '\'' && iter != originalData.end(); iter++)
								value.append(1, *iter);
						}
						AddAttribute(name, value, elementDataStack.top());
						for (; *iter == '\t' || *iter == '\n' || *iter == ' ' && iter != originalData.end(); iter++);
					}
				}
				if (*iter == '/') //empty element
				{
					iter++;
					elementStack.pop();
					elementDataStack.pop();
				}
			}
		}
		else //PCDATA or non-hypertext document
		{
			if (iter == originalData.begin())
				return false;
			else
			{
			}
		}
	}
	//Evaluate results
}

bool XmlFile::Unload()
{
	originalData.clear();
	delete []originalFilename;
	dtd->clear();
	root->clear();
	return true;
}

bool XmlFile::Save(const string filename)
{
	if (!ParseData(true))
		return false;

	ofstream file;
	if (filename.empty())
		file.open(originalFilename);
	else
		file.open(originalFilename);
	file.write(originalData.c_str(), originalData.size());
}

bool XmlFile::Revert()
{
	return ParseData(false);
}

XmlElement_Dyn<prefType>::XmlElement_Dyn(const string name): elementName(name)
{
}

XmlElement_Dyn<prefType>::~XmlElement_Dyn()
{
}

void XmlElement_Dyn<prefType>::clear()
{
	subElements.clear();
	attributes.clear();
	pcData.clear();
}
