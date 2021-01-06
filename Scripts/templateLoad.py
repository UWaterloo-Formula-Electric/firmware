from string import Template
class FSAETemplater:
    # Keys for locating template file
    # Format:
    #   fileKey: path
    TEMPLATE_MAP = {
        "INCLUDE_HEADERS_BEGIN": "templates/include_headers_begin.txt"
    }

    def __init__(self):
        # Templates which have already been loaded
        # Format:
        #   fileKey: templateData
        self.loadedTemplates = dict()

    # Get template from file, plug in data and return string data
    def load(self, fileKey, data={}):
        templateString = self.__getTemplate(fileKey)
        parsedTemplate = self.__parseTemplate(templateString, data)
        return parsedTemplate
    def __getTemplate(self, fileKey):
        if fileKey in self.loadedTemplates:
            return self.loadedTemplates[fileKey]
        return self.__loadTemplate(fileKey)
    def __loadTemplate(self, fileKey):
        with open(FSAETemplater.TEMPLATE_MAP[fileKey], 'r') as file:
            src = file.read()
            self.loadedTemplates[fileKey] = src
            return self.loadedTemplates[fileKey]
    def __parseTemplate(self, templateString, data={}):
        template = Template(templateString)
        return template.substitute(data)

if __name__ == '__main__':
    myTemplater = FSAETemplater()
    myOutput = myTemplater.load("INCLUDE_HEADERS")
    print(myOutput)