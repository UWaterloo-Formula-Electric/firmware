from string import Template
class FSAETemplater:
    # Keys for locating template file
    # Format:
    #   fileKey: path
    TEMPLATE_MAP = {
        "INCLUDE_HEADERS_BEGIN": "templates/include_headers_begin.txt",
        "INCLUDE_SOURCE_BEGIN": "templates/include_source_begin.txt",
        "END_HEADER": "templates/end_header.txt",
        "END_SOURCE": "templates/end_source.txt",
        "DBC_GIT_SOURCE": "templates/DBC_git_to_source.txt",
        "SIGNAL_RECIEVED_FUNC": "templates/signal_recieved_function.txt",
        "SIGNAL_SENDING_FUNC": "templates/signal_sending_function.txt",
        "SIGNAL_VAR_DECL_HEADER": "templates/signal_variable_and_declaration_header.txt",
        "SIGNAL_VAR_DECL_SOURCE": "templates/signal_variable_and_declaration_source.txt",
        "INDEX_TO_MUX_HEADER": "templates/index_to_mux_header.txt",
        "INDEX_TO_MUX_SOURCE": "templates/index_to_mux_source.txt",
        "MUX_TO_INDEX_HEADER": "templates/mux_to_index_header.txt",
        "MUX_TO_INDEX_SOURCE": "templates/mux_to_index_source.txt",
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