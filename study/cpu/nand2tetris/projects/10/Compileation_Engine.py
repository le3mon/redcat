import binascii
import JackTokenizer
class Compile:
    def __init__(self, input_path, output_path):
        self.w = open(output_path, "w")
        self.tokenizer = JackTokenizer.Tokenizer(input_path)
        self.tokenizer.has_more_tokens()

    
    def __del__(self):
        self.w.close()

    