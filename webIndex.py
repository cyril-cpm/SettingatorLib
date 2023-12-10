import os

def quoteFix(string:str):
    fixed = ""

    for c in string:
        if c == "\"":
            fixed += "\\\""
        elif c == "\n":
            fixed += ""
        elif c == "\\":
            fixed += "\\\\"
        elif c == "\t":
            fixed += ""
        elif c == " ":
            fixed += " "
        elif c == "\'":
            fixed == "\'"
        else:
            fixed += c

    return fixed

file = open("test.html", "r")

text = quoteFix(file.read())


preprocessor = "-DWEB_PAGE=\\\"" + text + "\\\""

#preprocessor = "-DWEB_PAGE=\\\"yo&quot;l&quot;o\\\""

print(preprocessor)
os.environ["PLATFORMIO_BUILD_FLAGS"] = preprocessor

#print(os.environ)

os.system("C:\\Users\\Cyril\\.platformio\\penv\\Scripts\\platformio.exe run --target upload")