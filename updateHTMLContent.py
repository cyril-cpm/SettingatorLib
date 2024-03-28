
def escapeChar(textString):
    escaped = ""

    for c in textString:
        if c == "\n":
            escaped += "\\n"
        elif c == "\r":
            escaped += "\\r"
        elif c == "\t":
            escaped += "\\t"
        elif c == "\"":
            escaped += "\\\""
        elif c == "\\":
            escaped += "\\\\"
        else:
            escaped += c

    return escaped

file = open("data/index.html")

escaped = escapeChar(file.read())

file.close()

sourceFile = open("src/HTTPServer.cpp", "r+")

updatedSourceFile = ""

for lineNumber, lineString in enumerate(sourceFile, 1):

    if "String HTML_CONTENT = " in lineString:
        updatedSourceFile += "String HTML_CONTENT = \"" + escaped + "\";\n"
    else:
        updatedSourceFile += lineString

sourceFile.seek(0)

sourceFile.write(updatedSourceFile)
