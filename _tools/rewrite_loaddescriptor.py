# -*- coding: utf-8 -*-
import io

path = r"C:\Users\Administrator\Documents\GitHub\stabledecompile\SexyAppFramework\DescParser.cpp"
with io.open(path, "r", encoding="utf-8", newline="") as f:
    text = f.read()

marker = "bool DescParser::LoadDescriptor(const std::string& theFileName)"
p_fs = text.find(marker)
assert p_fs != -1, "function marker not found"
ls_fs = text.rfind("\n", 0, p_fs) + 1

p_fc = text.find("p_fclose(aStream);", p_fs)
assert p_fc != -1, "p_fclose not found"
p_ret = text.find("return !hasErrors;", p_fc)
assert p_ret != -1, "return not found"
# closing '}' of the function is the '}' at the start of the line after 'return !hasErrors;'
p_close = text.find("\n}", p_ret)
assert p_close != -1, "closing brace not found"
func_end = p_close + 2  # include the '\n}'

fn = """bool DescParser::LoadDescriptor(const std::string& theFileName)
{
	mCurrentLineNum = 0;
	int aLineCount = 0;
	bool hasErrors = false;

	//Apparently VC6 doesn't have a clear() function for basic_strings
	//mError.clear();
	mError.erase();
	mError.erase(mError.begin());

	PFILE *aStream = p_fopen(theFileName.c_str(),"r");
	if (aStream==NULL)
		return false;	

	char aBuffChar = 0;

	// 豆包修复：剥离 UTF-8 BOM（EF BB BF）。
	// 中文年度版字体描述文件（如 BrianneTod*.txt）为 UTF-8 带 BOM 编码，
	// 若不去除 BOM，EF BB BF 会被当作首字符参与解析导致失败。
	int aPushback[3] = { 0, 0, 0 };
	int aPushbackCount = 0;
	{
		int aB0 = p_fgetc(aStream);
		int aB1 = p_fgetc(aStream);
		int aB2 = p_fgetc(aStream);
		if (aB0 == 0xEF && aB1 == 0xBB && aB2 == 0xBF)
		{
			// 命中 UTF-8 BOM：直接丢弃，从 BOM 之后继续解析
		}
		else
		{
			// 不是 BOM：按原顺序回放，保证与未改动前行为一致
			if (aB0 != EOF) aPushback[aPushbackCount++] = aB0;
			if (aB1 != EOF) aPushback[aPushbackCount++] = aB1;
			if (aB2 != EOF) aPushback[aPushbackCount++] = aB2;
		}
	}

	while (!p_feof(aStream))
	{		
		int aChar;
					
		bool skipLine = false;
		bool atLineStart = true;
		bool inSingleQuotes = false;
		bool inDoubleQuotes = false;
		bool escaped = false; 
		bool isIndented = false;

		for (;;)
		{
			if (aBuffChar != 0)
			{
				aChar = aBuffChar;
				aBuffChar = 0;
			}
			else if (aPushbackCount > 0)
			{
				aChar = aPushback[0];
				for (int i = 1; i < aPushbackCount; i++)
					aPushback[i - 1] = aPushback[i];
				aPushbackCount--;
			}
			else
			{
				aChar = p_fgetc(aStream);
				if (aChar==EOF)
					break;
			}
			
			if (aChar != '\\r')
			{
				if (aChar == '\\n')
					aLineCount++;

				if (((aChar == ' ') || (aChar == '\\t')) && (atLineStart))
					isIndented = true;

				if ((!atLineStart) || ((aChar != ' ') && (aChar != '\\t') && (aChar != '\\n')))
				{
					if (atLineStart)
					{
						if ((mCmdSep & CMDSEP_NO_INDENT) && (!isIndented) && (mCurrentLine.size() > 0))
						{
							// Start a new non-indented line
							aBuffChar = aChar;
							break;
						}

						if (aChar == '#')
							skipLine = true;

						atLineStart = false;
					}					

					if (aChar == '\\n')		
					{
						isIndented = false;
						atLineStart = true;				
					}

					if ((aChar == '\\n') && (skipLine))
					{
						skipLine = false;						
					}
					else if (!skipLine)
					{
						if (aChar == '\\\\' && (inSingleQuotes || inDoubleQuotes) && !escaped)
							escaped = true;
						else
						{
							if ((aChar == '\\'') && (!inDoubleQuotes) && (!escaped))
								inSingleQuotes = !inSingleQuotes;

							if ((aChar == '"') && (!inSingleQuotes) && (!escaped))
								inDoubleQuotes = !inDoubleQuotes;
							
							if ((aChar == ';') && (mCmdSep & CMDSEP_SEMICOLON) && (!inSingleQuotes) && (!inDoubleQuotes))
								break;
							
							if(escaped) // stay escaped for when this is actually parsed
							{
								mCurrentLine += '\\\\';
								escaped = false;
							}

							if (mCurrentLine.size() == 0)
								mCurrentLineNum = aLineCount + 1;

							mCurrentLine += aChar;
						}
					}
				}
			}
		}

		if (mCurrentLine.length() > 0)
		{
			if (!ParseDescriptorLine(mCurrentLine))
			{
				hasErrors = true;
				break;
			}

			//Apparently VC6 doesn't have a clear() function for basic_strings
			//mCurrentLine.clear();
			mCurrentLine.erase();
		}
	}

	//Apparently VC6 doesn't have a clear() function for basic_strings
	//mCurrentLine.clear();
	mCurrentLine.erase();
	mCurrentLineNum = 0;

	p_fclose(aStream);
	return !hasErrors;
}
"""

text = text[:ls_fs] + fn + text[func_end:]
with io.open(path, "w", encoding="utf-8", newline="\n") as f:
    f.write(text)
print("OK - LoadDescriptor function fully rebuilt")
