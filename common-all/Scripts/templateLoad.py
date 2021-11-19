from string import Template
import os
class FSAETemplater:
    # Keys for locating template file
    # Format:
    #   fileKey: path
    TEMPLATE_MAP = {
        "INCLUDE_HEADERS_BEGIN": "templates/file_begin/include_headers_begin.txt",
        "INCLUDE_SOURCE_BEGIN": "templates/file_begin/include_source_begin.txt",

        "END_HEADER": "templates/file_end/end_header.txt",
        "END_SOURCE": "templates/file_end/end_source.txt",

        "DBC_GIT_SOURCE": "templates/DBC/DBC_git_to_source.txt",

        "SIGNAL_RECEIVED_FUNC": "templates/signal/signal_received_function.txt",
        "SIGNAL_SENDING_FUNC": "templates/signal/signal_sending_function.txt",

        "SIGNAL_VAR_DECL_HEADER": "templates/signal/signal_variable_and_declaration_header.txt",
        "SIGNAL_VAR_DECL_SOURCE": "templates/signal/signal_variable_and_declaration_source.txt",

        "INDEX_TO_MUX_HEADER": "templates/mux/index_to_mux_header.txt",
        "INDEX_TO_MUX_SOURCE": "templates/mux/index_to_mux_source.txt",
        "MUX_TO_INDEX_HEADER": "templates/mux/mux_to_index_header.txt",
        "MUX_TO_INDEX_SOURCE": "templates/mux/mux_to_index_source.txt",

        "MULTIPLEX_RX_HEADER": "templates/messages/multiplexed_rx_header.txt",
        "MULTIPLEX_RX_SOURCE": "templates/messages/multiplexed_rx_source.txt",

        "VERSION_SEND_HEADER": "templates/messages/version_send_header.txt",
        "VERSION_SEND_SOURCE": "templates/messages/version_send_source.txt",

        "SETUP_CAN_FILTERS_SOURCE": "templates/filters/setup_can_filters_source.txt",
        "SETUP_CAN_FILTERS_HEADER": "templates/filters/setup_can_filters_header.txt",
        "SETUP_CAN_FILTERS_PARTIAL": "templates/filters/setup_can_filters_partial.txt",
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
        relPath = os.path.join(os.path.dirname(__file__),FSAETemplater.TEMPLATE_MAP[fileKey])
        with open(relPath, 'r') as file:
            src = file.read()
            self.loadedTemplates[fileKey] = src
            return self.loadedTemplates[fileKey]
    def __parseTemplate(self, templateString, data={}):
        template = Template(templateString)
        return template.substitute(data)
