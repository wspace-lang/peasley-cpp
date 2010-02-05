#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

using namespace std;

const string IMP_STACK          = "\x20";
const string IMP_ARITHMETIC  = "\x9\x20";
const string IMP_HEAP           = "\x9\x9";
const string IMP_FLOWC        = "\xA";
const string IMP_IO              = "\x9\xA";

// Stack manipulation
const string STACK_PUSH  = "\x20";  // one argument
const string STACK_DUPLICATE = "\xA\x20";
const string STACK_COPY  = "\x9\x20";  // one argument
const string STACK_SWAP = "\xA\x9";
const string STACK_POP   = "\xA\xA";
const string STACK_REMOVE = "\x9\xA"; // one argument

// Arithmetic
const string ARITHMETIC_ADD = "\x20\x20";
const string ARITHMETIC_SUB = "\x20\x9";
const string ARITHMETIC_MULTIPLICATION = "\x20\xA";
const string ARITHMETIC_DIVISION = "\x9\x20";
const string ARITHMETIC_MODULO = "\x9\x9";

// Heap access
const string HEAP_STORE = "\x20";
const string HEAP_RETRIEVE = "\x9";

// Flow control
const string FLOW_MARK = "\x20\x20";
const string FLOW_CALL_SUBROUTINE = "\x20\x9";
const string FLOW_JUMP = "\x20\xA";
const string FLOW_ZJUMP = "\x9\x20"; // jump if top of the stack is zero
const string FLOW_NJUMP = "\x9\x9"; // negative jump
const string FLOW_RETURN = "\x9\xA";
const string FLOW_ENDPROGRAM = "\xA\xA";

// IO
const string IO_OUTPUT_CHAR = "\x20\x20";
const string IO_OUTPUT_NUMBER = "\x20\x9";
const string IO_READ_CHAR = "\x9\x20";
const string IO_READ_NUMBER = "\x9\x9";

inline bool is_valid_ws(char cn){ return cn == '\x20' || cn == '\xA' || cn == '\x9'; }

typedef int (*command_execution)(void *);
struct command
{
  command_execution exec;
  int parameter_count;
  command() {}
  command(command_execution e, int p, const string &dename) : exec(e), parameter_count(p),
      dname(dename) {}
    
   const string &get_debug_name() { return dname; }
  private:
    string dname;
};


typedef map<string, command> instruction_set;
typedef map<string, instruction_set> imp_set;
typedef map<string, int> namesp;
typedef long long int WHSPARAM;

struct parameter
{
  string raw; 
  WHSPARAM val; 
};

struct token
{
  instruction_set *iset;

  command *exec;
  vector<parameter> parameters;
};


vector<WHSPARAM> stack;
map<int,  WHSPARAM> heap;


int stack_push(void *info)
{
  WHSPARAM number = *(WHSPARAM *)info;
  stack.push_back(number);
  return 1;
}
int stack_duplicate(void *)
{
  if(stack.empty()) return 0;
  WHSPARAM last_item = stack.back();
  stack.push_back(last_item);
  return 1;
}
int stack_copy(void *v)
{
  WHSPARAM slot = *(WHSPARAM *)v;
  if(slot < 0 || slot >= stack.size()) return 0;
  stack.push_back(stack[slot]);
  return 1;
}
int stack_swap(void *)
{
  if(stack.size() < 2) return 0;
  WHSPARAM last = stack.back();
  WHSPARAM lastmi = stack[stack.size() - 2];
  WHSPARAM temp;
  temp = last;
  last = lastmi;
  lastmi = temp;
  stack.pop_back();
  stack.pop_back();
  stack.push_back(lastmi);
  stack.push_back(last);
  return 1;
}
int stack_pop(void *)
{
  if(stack.empty()) return 0;
  stack.pop_back();
  return 1;
}
int stack_erase(void *w)
{
  int count = *(int *)w;
  if(stack.empty()) return 0;
  WHSPARAM last = stack.back();
  stack.pop_back();
  if(stack.size() < count) return 0;
  stack.erase(stack.begin(), stack.end() - count);
  return 1;
}

int* deliver()
{
  static int w[2];
  w[1] = stack[stack.size() - 1]; 
  w[0] =  stack[stack.size() - 2];
  stack.pop_back();
  stack.pop_back();
  return w;
}
int arithmetic_add(void *)
{
  if(stack.size() < 2) return 0;
  int *n = deliver();
  stack.push_back(n[0] + n[1]);
  return 1;
}
int arithmetic_sub(void *)
{
  if(stack.size() < 2) return 0;
  int *n = deliver();
  stack.push_back(n[0]  - n[1]);
  return 1;
}
int arithmetic_multiplication(void *)
{
  if(stack.size() < 2) return 0;
  int *n = deliver();
  stack.push_back(n[0] * n[1]);
  return 1;
}
int arithmetic_division(void *)
{
  if(stack.size() < 2) return 0;
  int *n = deliver();
  if(n[1] == 0) return 0;
  stack.push_back(n[0] / n[1]);
  return 1;
}
int arithmetic_modulo(void *)
{
  if(stack.size() < 2) return 0;
  int *n = deliver();
  stack.push_back(n[0] % n[1]);
  return 1;
}

int heap_store(void *)
{
  if(stack.size() < 2) return 0;
  WHSPARAM adr = stack[stack.size() - 2];
  WHSPARAM val = stack.back();
  heap[adr] = val;
  stack.pop_back();
  stack.pop_back();
  return 1;
}
int heap_retrieve(void *)
{
  if(stack.size() < 1) return 0;
  WHSPARAM adr = stack.back();
  stack.pop_back();
  stack.push_back(heap[adr]);
  return 1;
}

int dummy(void *)
{
  return 0;
}
int io_output_char(void *)
{
  WHSPARAM a = stack.back();
  cout << (char)a;
  stack.pop_back();
  return 1;
}
int io_output_number(void *)
{
  WHSPARAM a = stack.back();
  cout << a;
  stack.pop_back();
  return 1;
}
int io_read_char(void *)
{
  char c;
  cin.get(c);
  heap[stack.back()] = c;
  stack.pop_back();
  return 1;
}
int io_read_number(void *)
{
  WHSPARAM numb;
  cin >> numb;
  heap[stack.back()] = numb;
  stack.pop_back();
  return 1;
}

template<class rtype, class wtype>
rtype *detect(ifstream &data, wtype &f)
{
  string tf;
  while(!data.eof())
  {
    char c;
    data.read(&c, 1);
    if(!is_valid_ws(c)) continue;
    tf += c;
    
   typename wtype::iterator wfs = f.find(tf);
   if(wfs != f.end()) 
     return &wfs->second;
  }
  return 0;
}
string read_parameter(ifstream &data)
{
  string fg;
  while(!data.eof())
  {
    char c;
    data.read(&c, 1);
    if(c == '\xA') break;
    if(c == '\x20')
      fg += '0';
    else if(c == '\x9')
      fg += '1';
  }
  return fg;
}
WHSPARAM generate_value(const string &fstr)
{
  WHSPARAM val = 0;
  for(int j = 0; j < fstr.length(); j++)
    val |= fstr[fstr.length() - j - 1] == '1' ? (1 << j) : 0;
  
  if(fstr[0] == '1') val = ~val + 1;
  return val;
}

int jump(const namesp &space, string where)
{
  namesp::const_iterator kc = space.find(where);
  if(kc == space.end())
  {
    cout << "Jump to unknown space.";
    return -1;
  }
  return kc->second;
}

typedef instruction_set *(*deimp)(ifstream &, imp_set &);
typedef command *(*decom)(ifstream &, instruction_set &);

deimp detect_imp = detect<instruction_set, imp_set>;
decom detect_command = detect<command, instruction_set>;

void run_code(const vector<token> &code, const namesp &space)
{
  int current_instruction = 0;
  bool running = true;
  vector<int> function_stack;
  
  static vector<string> fcj; 
  fcj.push_back("flow_zjump(label)");
  fcj.push_back("flow_njump(label)");

  while(running && current_instruction < code.size())
  {
    token w = code[current_instruction];
    if(w.exec->get_debug_name() == "flow_endprogram()")
    {
      cout << "Finished executing script.";
      running = false;
      break;
    }

    if(w.exec->get_debug_name() == "flow_jump(label)")
      if((current_instruction = jump(space, w.parameters[0].raw)) < 0)
        return;

    if(!stack.empty() && find(fcj.begin(), fcj.end(), w.exec->get_debug_name()) != fcj.end())
    {
        int stack_last;
        stack_last = stack.back();
        stack.pop_back();
      
      if(w.exec->get_debug_name() == "flow_zjump(label)" )
        if(stack_last == 0)
          if((current_instruction = jump(space, w.parameters[0].raw)) < 0) 
            return;

      if(w.exec->get_debug_name() == "flow_njump(label)")
        if(stack_last < 0)
          if((current_instruction = jump(space, w.parameters[0].raw)) < 0)
            return;
    }
    
    if(w.exec->get_debug_name() == "flow_call_subroutine(label)")
    {
      function_stack.push_back(current_instruction + 1);
      current_instruction = jump(space, w.parameters[0].raw);
      if(current_instruction < 0) return;
    }
    
    if(w.exec->get_debug_name() == "flow_return()")
    {
      int where = function_stack.back();
      function_stack.pop_back();
      current_instruction = where;
      continue;
    }
    
    void *k = (w.parameters.empty() ? 0 : (void *)&w.parameters[0].val);
    w.exec->exec(k);
    current_instruction += 1;
  }
}

int main(int argc, char *argv[])
{
  if(argc < 2)
  {
    cout << "Usage: <filename>";
    return 1;
  }
  
  ifstream data(argv[1]);
  if(data.fail())
  {
    cout << "Error while trying to read file " << argv[1] << endl;
    return 2;
  }
  
   instruction_set stack_set;
   stack_set[STACK_PUSH] = command(stack_push, 1, "stack_push(value)");
   stack_set[STACK_COPY] = command(stack_copy, 1, "stack_copy(value)");
   stack_set[STACK_SWAP] = command(stack_swap, 0, "stack_swap()");
   stack_set[STACK_DUPLICATE] = command(stack_duplicate, 0, "stack_duplicate()");
   stack_set[STACK_POP]   = command(stack_pop, 0, "stack_pop()");
   stack_set[STACK_REMOVE] = command(stack_erase, 1, "stack_remove(value)");
  
  instruction_set arithmetic_set;
  arithmetic_set[ARITHMETIC_ADD]  = command(arithmetic_add, 0, "arithmetic_add()");
  arithmetic_set[ARITHMETIC_SUB] = command(arithmetic_sub, 0, "arithmetic_sub()");
  arithmetic_set[ARITHMETIC_MULTIPLICATION] = command(arithmetic_multiplication, 0, "arithmetic_multiplication()");
  arithmetic_set[ARITHMETIC_DIVISION] = command(arithmetic_division, 0, "arithmetic_division()");
  arithmetic_set[ARITHMETIC_MODULO] = command(arithmetic_modulo, 0, "arithmetic_modulo()");
  
  instruction_set heap_set;
  heap_set[HEAP_STORE]     = command(heap_store, 0, "heap_store(where, value)");
  heap_set[HEAP_RETRIEVE] =  command(heap_retrieve, 0, "heap_retrieve(from)");
  
  instruction_set flow_set;
  flow_set[FLOW_MARK] = command(dummy, 1, "flow_mark(label)");
  flow_set[FLOW_CALL_SUBROUTINE] = command(dummy, 1, "flow_call_subroutine(label)");
  flow_set[FLOW_JUMP] = command(dummy, 1, "flow_jump(label)");
  flow_set[FLOW_ZJUMP] = command(dummy, 1, "flow_zjump(label)");
  flow_set[FLOW_NJUMP] = command(dummy, 1, "flow_njump(label)");
  flow_set[FLOW_RETURN] = command(dummy, 0, "flow_return()");
  flow_set[FLOW_ENDPROGRAM] = command(dummy, 0, "flow_endprogram()");
  
  instruction_set io_set;
  io_set[IO_OUTPUT_CHAR] = command(io_output_char, 0, "io_output_char()");
  io_set[IO_OUTPUT_NUMBER] = command(io_output_number, 0, "io_output_number()");
  io_set[IO_READ_CHAR] = command(io_read_char, 0, "io_read_char()");
  io_set[IO_READ_NUMBER] = command(io_read_number, 0, "io_read_number");

  imp_set is;
  is[IMP_STACK]       = stack_set; 
  is[IMP_ARITHMETIC] = arithmetic_set;
  is[IMP_HEAP]         = heap_set;
  is[IMP_FLOWC]       = flow_set;
  is[IMP_IO]             = io_set;
  
  
  vector<token> code;
  namesp global_namespace;

 bool syntax_valid = true;
 int instruction_index = 0;

  while(syntax_valid && !data.eof())
  {
    char cn = data.peek();
    if(!is_valid_ws(cn))
    {
      data.get(cn);
      continue;
    }
    
    token cur_token;
    instruction_set *w = detect_imp(data, is);
    if(!w)
    {
      cout << "Error while detecting imp type. exiting!";
      syntax_valid = false;
      break;
    }
    
    command *wm = detect_command(data, *w);
    if(!wm)
    {
      cout << "Command not recognized from this specified instruction set.";
      syntax_valid = false;
      break;
    }
    
    cur_token.iset = w;
    cur_token.exec = wm;
    
    string param;
    for(int i = 0; i < wm->parameter_count; i++)
    {
      parameter p;
      p.raw = (param = read_parameter(data));
      p.val = generate_value(p.raw);
      cur_token.parameters.push_back(p);
    }

    if(wm->get_debug_name() == "flow_mark(label)")
        global_namespace[param] = instruction_index;

    code.push_back(cur_token);
    instruction_index++;
  }

  if(syntax_valid) 
    run_code(code, global_namespace);
  data.close();
  return 0;
}
