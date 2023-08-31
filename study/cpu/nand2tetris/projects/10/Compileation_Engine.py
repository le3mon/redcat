import binascii
import JackTokenizer
class Compile:
    def __init__(self, input_path, output_path):
        self.w = open(output_path, "w")
        self.tokenizer = JackTokenizer.Tokenizer(input_path)
        
        # for i in range(10):
        self.tokenizer.advance()
            # print(self.tokenizer.get_token())
            # print(self.tokenizer.get_token_type())

    
    def __del__(self):
        self.w.close()

    