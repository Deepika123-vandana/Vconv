#include "systemc.h"
#include <fstream>
#include <string>
#include <iostream>
#include <vector>
#include <iomanip>
#include <sstream>
#include <cstring>
using namespace sc_core;

template <int conv_lsize, int conv_ksize, int conv_stride, int conv_pad>
SC_MODULE(MAIN_DESIGN) {
    // INPUT SIGNALS
    sc_in<bool> clk;
    sc_in<bool> rst;
    sc_in<bool> s_rst;
    sc_in<bool> conv_en;
    sc_in<bool> inp_last;
    sc_in<bool> inp_val;
    sc_in<bool> op_rdy;
    sc_in<uint32_t> inp_data;

    sc_in<int> NUM_MAC;
    sc_in<int> NUM_CU;
    sc_in<int> NUM_CE;

    // OUTPUT SIGNALS
    sc_out<bool> inp_rdy;
    sc_out<bool> op_last;
    sc_out<bool> op_val;
    sc_out<uint32_t> out_data;

    sc_signal<bool, SC_MANY_WRITERS> inp_sig;
    sc_signal<bool, SC_MANY_WRITERS> op_last_sig;
    sc_signal<bool, SC_MANY_WRITERS> op_val_sig;
    sc_signal<uint32_t, SC_MANY_WRITERS> op_data_sig;

    // INTERNAL SIGNALS
    int padded_size = conv_lsize + 2 * conv_pad; // Size for line_with_pad memory
    int output_size = ((conv_lsize - conv_ksize + (2 * conv_pad)) / conv_stride) + 1;  // Size for output data memory
    float temp_data; // To store convolution temporary data
    int data_count;  // To count data inputs received
    int i, j, m, n = 0;   // Internal looping variables
    int index = 0;     // For counting no. of data items
    int data_out_counter = 0; // To count no. of data items sent to output port
    int out_row, out_col = 0;  // To access output data to out_data port
    int load_row;
    int col_str, col = 0;
    int k;

    // MEMORIES FOR STORING DATA
    std::vector<std::vector<float>> kernel;
    std::vector<std::vector<float>> line_with_pad;
    std::vector<std::vector<float>> out;
    sc_fifo<float> out_fifo;

    int addition_delay = 11;
    int multiplication_delay = 8;
    int inp_channels=1;
    int kernel_count=1;
    int no_of_conv=inp_channels*kernel_count;
    int no_of_matrix_mul=no_of_conv*output_size*output_size;
    int no_of_multiplications=no_of_matrix_mul*conv_ksize*conv_ksize; 

    sc_signal<bool> inp_rdy_flag;
    // EVENTS
    sc_event load_event;  // To notify loading data has completed
    sc_event conv_event;  // To give output after convolution is done
    sc_event rst_event;   // To notify to load_data process for reset is high
    sc_event col_event;
    sc_event filled;

    SC_CTOR(MAIN_DESIGN) : kernel(conv_ksize, std::vector<float>(conv_ksize)), line_with_pad(padded_size, std::vector<float>(padded_size)), out(output_size, std::vector<float>(output_size)), out_fifo(conv_lsize * conv_lsize), temp_data(0) {
        // RESET PROCESS
        SC_METHOD(RESET);
        sensitive << rst << s_rst;

        // LOADING DATA PROCESS
        SC_THREAD(load_data);
        sensitive << clk.pos() << rst_event;
        dont_initialize();

        // CONVOLUTION PROCESS
        SC_THREAD(conv);
        sensitive << load_event;
        dont_initialize();

        // OUTPUT READING PROCESS
        SC_THREAD(Control_Delay);
        sensitive << col_event;
        dont_initialize();

        SC_METHOD(process);
        sensitive << inp_sig << op_val_sig << op_last_sig << op_data_sig;
        dont_initialize();
    }

    void process() {
        inp_rdy.write(inp_sig.read());
        op_last.write(op_last_sig.read());
        op_val.write(op_val_sig.read());
        out_data.write(op_data_sig.read());
        next_trigger();
    }

    void RESET() {
        if (rst || s_rst) {
            temp_data = 0;
            data_count = 0;
            i = j = m = n = 0;
            index = 0;
            data_out_counter = 0;
            out_row = out_col = 0;
            op_data_sig.write(0);
            op_val_sig.write(0);
            op_last_sig.write(0);
            inp_sig.write(1);
            std::cout << "In Reset State"<<"rst"<<rst<<"inp_rdy"<< inp_rdy<<"inp_val"<<inp_val << " op_last=" << op_last << " op_val=" << op_val << std::endl;
            // Reset internal data
            kernel.assign(conv_ksize, std::vector<float>(conv_ksize, 0.0f));
            line_with_pad.assign(padded_size, std::vector<float>(padded_size, 0.0f));
            out.assign(output_size, std::vector<float>(output_size, 0.0f));
            rst_event.notify(SC_ZERO_TIME);
        }
    }

    void Control_Delay() {
        while (true) {
            // Formula to calculate how many number of rows should be loaded
            if ((col_str + conv_stride > conv_pad) && ((col_str - conv_pad + conv_ksize) < conv_lsize)) {
                if ((col_str + conv_stride - conv_pad) > conv_stride) {
                    load_row = conv_stride;
                    std::cout << "Load row=" << load_row << " in if if state " << std::endl;
                } else {
                    load_row = col_str + conv_stride - conv_pad;
                    std::cout << "Load row=" << load_row << " in if else state " << std::endl;
                }
            }
            inp_sig.write(1);
            std::cout << "In Control Delay before load inp_rdy=" << inp_rdy.read() << " delay start=" << sc_time_stamp() << std::endl;
            std::cout << "load row=" << load_row << std::endl;
            for (int dd = 0; dd < conv_lsize * load_row + 1; dd++) {
                wait(clk.posedge_event());
            }
            filled.notify();
            inp_sig.write(0);
            wait(0, SC_NS);
            std::cout << "In Control Delay after load inp_rdy=" << inp_rdy.read() << " delay end=" << sc_time_stamp() << std::endl;
            wait();
        }
    }
    void load_data() {
        while (true) {
            wait(SC_ZERO_TIME);

            if ((inp_val || inp_last) && (inp_rdy)) {
                if (index < (conv_ksize * conv_ksize)) {
                    kernel[i][j] = *reinterpret_cast<const float*>(&inp_data.read());
                    std::cout << sc_time_stamp() << " Kernel[" << i << "][" << j << "] = " << kernel[i][j] << "  inp_val=" << inp_val << "  inp_last=" << inp_last << std::endl;

                    j++;
                    if (j == conv_ksize) {
                        j = 0;
                        i++;
                    }
                    if (i == conv_ksize) {
                        i = 0;
                    }
                } else {
                    line_with_pad[m + conv_pad][n + conv_pad] = *reinterpret_cast<const float*>(&inp_data.read());
                    std::cout << sc_time_stamp() << " line[" << m + conv_pad << "][" << n + conv_pad << "] = " << std::hex << inp_data.read() << std::dec << "  inp_val=" << inp_val << "  inp_last=" << inp_last << std::endl;

                    n++;
                    if (n == conv_lsize) {
                        n = 0;
                        m++;
                    }
                    if (m == conv_lsize) { m = 0; }
                }

                index++;

                if (index == (conv_ksize * conv_ksize) + ((conv_ksize + conv_stride) * conv_lsize)) {
                    std::cout << "index=" << index << std::endl;
                    load_event.notify();
                    wait(0, SC_NS);

                    inp_sig.write(0);
                    wait(0, SC_NS);

                    std::cout << "Main inp_rdy=" << inp_rdy << std::endl;
                }
            }
            if (index == (conv_lsize * conv_lsize) + (conv_ksize * conv_ksize)) {
                index=0;
                wait(clk.posedge_event());
               
            }

            wait(clk.posedge_event());
        }
    }

void conv() {
    while (true) {
        // Wait until the required amount of data is loaded
         while (!conv_en) { wait(clk.posedge_event()); }
        std::cout << "TOTAL_MAC=" << NUM_CE * NUM_MAC * NUM_CU << std::endl;
        for (col_str = 0, col = 0; col_str < padded_size - conv_ksize, col < output_size; col_str = col_str + conv_stride, col++) {
            std::cout << "col_str=" << col_str << std::endl;
            col_event.notify();
              if(col_str>=2){
            wait(filled);}
            for (int row_str = 0, row = 0; row_str < padded_size - conv_ksize, row < output_size; row_str += conv_stride, row++) {
                for (int i = col_str, x = 0; i < col_str + conv_ksize, x < conv_ksize; i++, x++) {
                    for (int j = row_str, y = 0; j < row_str + conv_ksize, y < conv_ksize; j++, y++) {
                   
                       
                         temp_data += line_with_pad[i][j] * kernel[x][y];
            //    std::cout<<sc_time_stamp()<<std::endl;
                        int total_macs = NUM_CE * NUM_CU * NUM_MAC;
                int total_mult_delay = multiplication_delay / total_macs;
                int total_add_delay = addition_delay / total_macs;
                int total_delay = total_mult_delay + total_add_delay;
                int required_cycle=no_of_multiplications*addition_delay/(total_macs); 
                    
            for (int delay = 0; delay < addition_delay; delay++) {
                    wait(clk.posedge_event());
                }
             
                    }
                }
                op_data_sig = *reinterpret_cast<uint32_t*>(&temp_data);

                out[col][row] = temp_data;
                out_fifo.write(temp_data);
                temp_data = 0;


                 
               op_val_sig.write(0); 
               wait(clk.posedge_event()); 
                 op_val_sig.write(1);
               // wait(clk.posedge_event()); 
          std::cout << sc_time_stamp() << " Output Data: " << std::hex << out_data.read() << std::dec << std::endl;
                      
        std::cout << sc_time_stamp() << " out[" << col << "][" << row << "]=" << out[col][row] << std::endl;
              
                

                if (out_fifo.num_available() == (output_size * output_size)) {
                    wait(clk.posedge_event());
                    op_last_sig.write(1);
                }
                if (out_fifo.num_available() == (output_size * output_size)) {
                    std::cout << sc_time_stamp() << " entered final stage" << std::endl;
                    break;
                }
            }
        }

        conv_event.notify();
       wait();
    }
}
};


    SC_MODULE(TB){
       sc_clock clk;
    
    sc_clock clk3;
    
  //sc_signal<float>sig1;
  //sc_signal<float>sig2;
  //sc_signal<double> sig3; 

 sc_signal<bool> rst;
  sc_signal<bool> s_rst;
  sc_signal<bool> conv_en;
  sc_signal<bool> inp_last;
  sc_signal<bool> inp_val;
  sc_signal<bool> op_rdy;
  sc_signal<uint32_t> inp_data;
  sc_signal<bool> inp_rdy;
  sc_signal<bool> op_last;
  sc_signal<bool> op_val;
  sc_signal<uint32_t> out_data;
  


    sc_signal<int> NUM_CE;
  sc_signal<int> NUM_CU;
  sc_signal<int> NUM_MAC;
 
sc_event mod1_done;
 sc_trace_file* file = sc_create_vcd_trace_file("trace");
   MAIN_DESIGN<224,7,2,3>* mod1;     
 

   SC_CTOR(TB):clk("clk",1,SC_NS),clk3("clk3",2,SC_NS){ 
    mod1 = new MAIN_DESIGN<224,7,2,3>("MAIN_DESIGN");
  

    // Connect modules to signals
    mod1->clk(clk);
    mod1->rst(rst);
    mod1->s_rst(s_rst);
    mod1->conv_en(conv_en);
    mod1->inp_last(inp_last);
    mod1->inp_val(inp_val);
    mod1->op_rdy(op_rdy);
    mod1->inp_data(inp_data);
    mod1->inp_rdy(inp_rdy);
    mod1->op_last(op_last);
    mod1->op_val(op_val);
    mod1->out_data(out_data);
    
    
    mod1->NUM_MAC(NUM_MAC);
    mod1->NUM_CU(NUM_CU);
    mod1->NUM_CE(NUM_CE);



          
        SC_THREAD(run);
        SC_THREAD(out);
        
        sc_trace(file, clk, "clk");
    sc_trace(file, rst, "rst");
    sc_trace(file, s_rst, "s_rst");
    sc_trace(file, conv_en, "conv_en");
    sc_trace(file, inp_last, "inp_last");
    sc_trace(file, inp_val, "inp_val");
    sc_trace(file, op_rdy, "op_rdy");   
    sc_trace(file, inp_data, "inp_data");
    sc_trace(file, inp_rdy, "inp_rdy");
    sc_trace(file, out_data, "out_data");
    sc_trace(file, op_val, "op_val");
    sc_trace(file, op_last, "op_last");
    
  }

  void run() {
      std::ifstream hex_file0("/home/admin1/Pictures/systemc/examples/vconv/k_0.hex");
    std::string line0;
    uint32_t data0;
     // 224x224 matrix

      std::vector<uint32_t> data_matrix1(121);

    for (int i = 0; i < 49; ++i) {
       
            if (std::getline(hex_file0, line0)) {
                std::stringstream ss;
                ss << std::hex << line0;  // Convert hex string to integer
                ss >> data0;

                // Store the data in the 2D vector
                data_matrix1[i] = data0;
            } 
        
    }

    hex_file0.close();
    
     std::ifstream hex_file("/home/admin1/Pictures/systemc/examples/vconv/l_0.hex");
    std::string line;
    uint32_t data;
 
 // 224x224 matrix
std::vector<std::vector<uint32_t>> data_matrix(224, std::vector<uint32_t>(224, 0));
   
    for (int i = 0; i < 224; ++i) {
        for (int j = 0; j < 224; ++j) {
            if (std::getline(hex_file, line)) {
                std::stringstream ss;
                ss << std::hex << line;  // Convert hex string to integer
                ss >> data;

                // Store the data in the 2D vector
                data_matrix[i][j] = data;
            } 
        }
    }

    hex_file.close();

             rst.write(1);
             wait(10,SC_NS);
             rst.write(0);
        
                            inp_val.write(1);
            
             NUM_CE=1;
             NUM_CU=1;
             NUM_MAC=1;
           /*   std::vector<uint32_t> kernel_data = {
              
              0xbce7f596,0xbbdd9904,0x3c8ad34a,0x3ce5106e,0x3cce8359,0x3be584cc,0xbc7dbf12,0xbc9206b6,0xbc9b50c2,0xbc278789,0x3bca0267,
              0xbd06f71d,0xbc51c81f,0x3c5fbff1,0x03cddf7a0,0x3cfdd92f,0x3c4bfb30,0xbc7f195e,0xbcc595ba,0xbca8c491,0xb9c049cf,0x3cce644b,
              0xbd1cf802,0xbc85d619,0x3c8cbbb7,0x3cc74787,0x3d064be4,0x3bf08b2c,0xbccb231e,0xbcd75dbc,0xbcc2e0d5,0x3afdfa91,0x3cfe3b99,
              0xbd0e0946,0xbc842910,0x3c9e9424,0x3d0b6ed3,0x3d228a51,0xbae4885e,0xbd0907ba,0xbd25be69,0xbcf1be2f,0x3b631ad8,0x3d1a952d,
              0xbcf5a283,0xbbf3a5b7,0x3ca423ec,0x3d2334ff,0x3d06a628,0xbbb41adf,0xbd35bf98,0xbd31e5e0,0xbcc680ca,0x3c6779b8,0x3d143d29,
              0xbcd34b48,0xbad85c86,0x3cddbfba,0x3d32928b,0x3ce83386,0xbc32eff9,0xbd248167,0xbd22d279,0xbca8ad7a,0x3c3b669f,0x3d269eb7,
              0xbcd614a3,0x3b8efbe2,0x3cb1b0ff,0x3d10c721,0x3ca905db,0xbc228d83,0xbd1ce1f3,0xbd088929,0xbc9d3428,0x3c2cd321,0x3d157413,
              0xbcaadcaa,0x3b81b900,0x3cc9f56d,0x3cde54e0,0x3c5b36fd,0xbc8b6ebf,0xbced108e,0xbd065fa2,0xbc4ecf1c,0x3c3d0dd0,0x3d1c00e3,
              0xbc591e86,0x3c451e60,0x3c72d75b,0x3ca7e4e7,0x3c050f4b,0xbc856825,0xbce72abb,0xbcb04e2f,0xbc331760,0x3c354547,0x3d051078,
              0xbc47ffc5,0x3c06b924,0x3c8b6a62,0x3c8482f1,0x3b39c656,0xbcad6e39,0xbcb8896d,0xbc4cd03c,0xbc0a0193,0x3c3c2afd,0x3cfce1f5,
              0xbb5ad2d3,0x3c6d2313,0x3c1bf3be,0x3c02213f,0xba129435,0xbc62a85b,0xbc95ad5a,0xbc195d7e,0xbc2447b7,0x3bdb1dad,0x3cad96f6
       
        };*/

wait(clk.posedge_event());
             
         
        for (int i = 0; i < 49; i++) {
       // while(!inp_rdy){wait(clk.posedge_event());}
             //wait(clk.posedge_event());
            inp_data = data_matrix1[i];
            wait(clk.posedge_event());
        }
 conv_en.write(1);
             
			 //inp_val.write(0);//inp_data=0xfffffff;wait(5, SC_NS);
               inp_val.write(1);
             //wait(clk.posedge_event());
             int counter=0;
             for (int i = 0; i < 224; i++) {
             for(int j=0;j<224;j++){
             while(!inp_rdy){wait(clk.posedge_event());}

             inp_data = data_matrix[i][j];
             std::cout<<"inp_data"<<i<<j<<std::hex<<data_matrix[i][j]<<std::dec<<std::endl;
         
            if(counter==50175){
           // wait(clk.posedge_event());
            inp_last.write(1);
            
            }
            
             wait(clk.posedge_event());
             counter++;
             
             }
             }
             
             
         } 
   void out() {
       int count=0;
       while (count<12544) {


        wait(op_val.posedge_event());

        // Print message when new data is detected
        std::cout << "New data available at " << sc_time_stamp() << std::endl;

        // Indicate that data is ready for processing
        op_rdy.write(1);

        // Print debug information
        std::cout << "out_val " << op_val.read()
                  << " out_last " << op_last.read()
                  << " output_data " << std::hex << out_data.read() << std::endl;

        // Wait for processing time
        wait(10, SC_NS);

        // Reset op_rdy after processing
        

        // Print processing end time
        std::cout << sc_time_stamp() << " Processing ended" << std::endl;   
        count++;
    }
    std::cout<<" endd "<<std::endl;
    sc_close_vcd_trace_file(file);
}
};
    
    
    int sc_main(int argc, char* argv[]) {
 std::ofstream log_file("/home/poojitha/SystemC/my_log.txt");
  std::streambuf *cout_buf = std::cout.rdbuf();
  std::cout.rdbuf(log_file.rdbuf());
   //sc_trace_file* file = sc_create_vcd_trace_file("trace");
    TB tb("tb");
    sc_start();
    // Restore original buffer (optional)
  std::cout.rdbuf(cout_buf);
  log_file.close();

	    //this is for test
    return 0;
}
sc_time MAX_SIM_TIME(10, SC_SEC);  // Set maximum simulation time to 10 seconds

SC_MODULE(SimulationControl) {
    SC_CTOR(SimulationControl) {
        SC_THREAD(run_simulation); // Register the thread
    }

    void run_simulation() {
        while (sc_time_stamp() < MAX_SIM_TIME) {
            wait(1, SC_MS); // Wait for 1 millisecond
        }
        sc_stop(); // Stop the simulation after MAX_SIM_TIME
    }
};

int sc_main(int argc, char* argv[]) {
    // Instantiate the SimulationControl module
    SimulationControl sim_control("SimControl");

    // Other modules and simulation setup

    sc_start();  // Start the simulation
    return 0;
}
