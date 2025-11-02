
#include <stdio.h>
#include <chrono>
#include <thread>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "CommonTypes.h"
#include "FileUtils.cpp"
#include "PrintData.cpp"

#define WIDTH  800
#define HEIGHT 800

// NOTE: Uncomment the following line for GL error handling
//#define GL_DEBUG

#ifdef GL_DEBUG
#define GLCALL(function) \
   { \
      GLenum error = GL_INVALID_ENUM; \
      while (error != GL_NO_ERROR) \
      { \
         error = glGetError(); \
      } \
      function; \
      error = glGetError(); \
      if (error != GL_NO_ERROR) \
      { \
         fprintf(stderr, "OpenGL Error: GL_ENUM(%d) at %s:%d\n", error, __FILE__, __LINE__); \
      } \
   }
#else
#define GLCALL(function) function;
#endif

std::vector<T1553Record> records[32][2];

float extract_time(const char* p)
{
   int d, h, m, s;
   double frac;
   // We only use m and s + frac
   sscanf(p, "%d:%d:%d:%d.%lf", &d, &h, &m, &s, &frac);
   frac /= 1000000.0;
   float time = ((float)m * 60.0) + (float)s + (float)frac;
   printf("Time: %fs\n", time);
   return time;
}

void print_record(const T1553Record& Record)
{
   printf("===================================\n");
   printf(" 1553 Record\n");
   printf("===================================\n");
   printf(" Record:  %d\n", Record.Record);
   printf(" Time:    %f\n", Record.Time);
   printf(" Channel: %d\n", Record.Channel);
   printf(" Bus:     %d\n", Record.Spare);
   printf(" Cmd:     0x%04x (%d, %c, %d, %d)\n",
          Record.Data.CommandWord,
          Record.Data.CommandWord.RemoteTerminal,
          Record.Data.CommandWord.Transmit ? 'T' : 'R',
          Record.Data.CommandWord.Subaddress,
          Record.Data.CommandWord.WordCount);
   printf(" Status:  0x%04x\n", Record.Data.StatusWord);
   printf(" Data Words:\n%s\n", CPrintData::GetDataAsString((char*)Record.Data.Words, sizeof(Record.Data.Words)));
   printf("===================================\n");
}

void parse_text_file()
{
   u64 record_count = 0;
   TBuffer buffer = ReadEntireFile("25kdama.txt");
   //TBuffer buffer = ReadEntireFile("25kdama.txt.small");
   char* p = (char*)buffer.Data;
   char* end = p + buffer.Size;

   int prev_rt = -1;

   for (int i = 0; i < 32; i++)
   {
      for (int j = 0; j < 2; j++)
      {
         records[i][j].reserve(5000);
      }
   }

   while (p < end)
   {
      T1553Record r = {};

      // Record number
      p = strstr(p, "Record:");
      if (p)
         sscanf(p, "Record: %d", &r.Record);
      else
         break;

      // Time
      p = strstr(p, "T=");
      if (p)
      {
         r.Time = extract_time(p + 2);
         p += 2;
      }

      // Command word decode: example: | Cwd1 = 7512 (14,T,08,18)
      p = strstr(p, "| Cwd1 =");
      if (p)
      {
         p += 9;
         uint16_t val;
         if (sscanf(p, "%hx", &val) == 1)
         {
            *((uint16_t*)&r.Data.CommandWord) = val;
         }
      }

      // Status word decode
      p = strstr(p, "| Swd1 =");
      if (p)
      {
         p += 9;
         uint16_t val;
         if (sscanf(p, "%hx", &val) == 1)
         {
            *((uint16_t*)&r.Data.StatusWord) = val;
         }
      }

      // Data Words: find all `0xFFFF`
      int num_words = r.Data.CommandWord.WordCount;
      if (num_words == 0)
         num_words = 32;

      char* next_rec = strstr(p, "Record:");

      for (int i = 0; i < num_words; i++)
      {
         p = strstr(p, "0x");

         if (next_rec && p > next_rec)
         {
            p = next_rec;
            break;
         }

         if (p)
         {
            uint16_t val;
            if (sscanf(p, "0x%hx", &val) == 1)
            {
               r.Data.Words[i] = val;
            }
            p += 2; // continue scanning forward
         }
      }

      u16 rt = r.Data.CommandWord.RemoteTerminal;
      u16 sa = r.Data.CommandWord.Subaddress;
      u16 tx = r.Data.CommandWord.Transmit;
      records[sa][tx].push_back(r);

      if (rt != prev_rt)
      {
         printf("Got RT %d\n", rt);
         prev_rt = rt;
      }

      //print_record(r);
      record_count++;

      if (!next_rec)
         break;
   }

   u64 total_records = 0;

   for (int i = 0; i < 32; i++)
   {
      for (int j = 0; j < 2; j++)
      {
         total_records += records[i][j].size();
      }
   }

   printf("Total 1553 records: %ld %ld\n", total_records, record_count);
}

void write_binary_file()
{
   std::ofstream file("25kdama.bin", std::ios::binary);

   for (int i = 0; i < 32; i++)
   {
      for (int j = 0; j < 2; j++)
      {
         u16 sa = i;
         u16 tx = j;
         s32 size = records[i][j].size();
         file.write((char*)&sa, sizeof(sa));
         file.write((char*)&tx, sizeof(tx));
         file.write((char*)&size, sizeof(size));
         if (size > 0)
            file.write((char*)records[i][j].data(), sizeof(T1553Record)*size);
      }
   }
}

void parse_binary_file()
{
   std::ifstream file("25kdama.bin", std::ios::binary);

   for (int i = 0; i < 32; i++)
   {
      for (int j = 0; j < 2; j++)
      {
         u16 sa;
         u16 tx;
         s32 size;
         file.read((char*)&sa, sizeof(sa));
         file.read((char*)&tx, sizeof(tx));
         file.read((char*)&size, sizeof(size));
         if (size > 0)
         {
            size_t buffer_size = sizeof(T1553Record)*size;
            char* buffer = new char[buffer_size];

            file.read(buffer, buffer_size);

            records[sa][tx].reserve(size);
            for (int k = 0; k < size; k++)
            {
               int idx = sizeof(T1553Record)*k;
               T1553Record* record = (T1553Record*)&buffer[idx];
               records[sa][tx].push_back(*record);
            }
         }
      }
   }

   u64 total_records = 0;

   for (int i = 0; i < 32; i++)
   {
      for (int j = 0; j < 2; j++)
      {
         total_records += records[i][j].size();
      }
   }

   printf("Total 1553 records: %ld\n", total_records);
}

float get_end_time()
{
   float end_time = 0.0;

   for (int i = 0; i < 32; i++)
   {
      for (int j = 0; j < 2; j++)
      {
         ssize_t end = records[i][j].size() - 1;
         if (end != -1 && records[i][j][end].Time > end_time)
            end_time = records[i][j][end].Time;
      }
   }

   return end_time;
}

const char* TxStr[] =
{
   "Rx",
   "Tx"
};

int main(int argc, char* argv[])
{
   GLFWwindow* window = nullptr;

   //parse_text_file();
   //write_binary_file();
   parse_binary_file();

   float curr_time = 0.0;
   float end_time = get_end_time();

   // initialize glfw
   if (!glfwInit())
      return 0;

   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

   // Create window
   window = glfwCreateWindow(WIDTH, HEIGHT, "ARC-210 Recording", NULL, NULL);

   if (!window)
   {
      glfwTerminate();
      return 0;
   }

   // make the window's context current
   glfwMakeContextCurrent(window);

   // use glad to load OpenGL function pointers
   if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
   {
      printf("Error: Failed to initialize GLAD.\n");
      glfwTerminate();
      return 0;
   }

   glfwSetWindowSize(window, WIDTH, HEIGHT);

   // Setup Dear ImGui
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   ImGui::StyleColorsDark();

   // Setup Platform/Render backends
   ImGui_ImplGlfw_InitForOpenGL(window, true);
   ImGui_ImplOpenGL3_Init("#version 330");

   // Make the window visible
   glfwShowWindow(window);

   // Initialize opengl
   GLCALL(glClearColor(0.5, 0.5, 0.5, 1.0));

   // enable blending
   GLCALL(glEnable(GL_BLEND));
   GLCALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

   // set frame rate to 60 Hz
   using framerate = std::chrono::duration<double, std::ratio<1, 60>>;
   auto frame_time = std::chrono::high_resolution_clock::now() + framerate{1};

   while (window)
   {
      // Poll events
      glfwPollEvents();

      if (glfwWindowShouldClose(window))
      {
         glfwTerminate();
         window = nullptr;
         break;
      }

      GLCALL(glClear(GL_COLOR_BUFFER_BIT));

      // Start the Dear ImGui frame
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      ImGui::Begin("1553 Data");

      ImGui::BeginChild("Time", ImVec2(0, 80), ImGuiChildFlags_Borders);

      ImGui::Text("End Time: %fs", end_time);
      ImGui::SliderFloat("Current Time", &curr_time, 0.0, end_time, "%.6f");
      if (ImGui::Button("Back"))
      {
         curr_time -= 0.1f;
         if (curr_time < 0.0f)
            curr_time = 0.0f;
      }
      ImGui::SameLine();
      if (ImGui::Button("Forward"))
      {
         curr_time += 0.1f;
         if (curr_time > end_time)
            curr_time = end_time;
      }

      ImGui::EndChild();
      
      ImGui::BeginChild("1553", ImVec2(0, 0), ImGuiChildFlags_Borders);

      for (int i = 0; i < 32; i++)
      {
         std::string subaddress_label = "SA ";
         subaddress_label += std::to_string(i);

         if (ImGui::CollapsingHeader(subaddress_label.c_str()))
         {
            for (int j = 0; j < 2; j++)
            {
               std::string label = std::string(TxStr[j]) +
                                   ": " + std::to_string(records[i][j].size()) +
                                   "##" + subaddress_label;

               if (ImGui::TreeNode(label.c_str()))
               {
                  // Get current index
                  int idx = 0;
                  for (const auto& record : records[i][j])
                  {
                     if (record.Time == curr_time)
                        break;

                     if (record.Time > curr_time)
                     {
                        idx -= 1;
                        break;
                     }

                     idx++;
                  }

                  if (ImGui::Button("Prev"))
                  {
                     if (idx > 0)
                     {
                        idx--;
                        curr_time = records[i][j][idx].Time;
                     }
                  }
                  ImGui::SameLine();
                  if (ImGui::Button("Next"))
                  {
                     if (idx < (int)records[i][j].size() - 1)
                     {
                        idx++;
                        curr_time = records[i][j][idx].Time;
                     }
                  }

                  if (ImGui::Button("Prev Diff"))
                  {
                     if (idx > 0)
                     {
                        int start_idx = idx;

                        if (start_idx >= records[i][j].size())
                           start_idx = records[i][j].size() - 1;

                        for (int k = start_idx; k >= 0; k--)
                        {
                           bool diff = false;
                           const auto& curr_rec = records[i][j][start_idx];
                           const auto& new_rec = records[i][j][k];

                           diff = memcmp(&curr_rec.Data.CommandWord,
                                         &new_rec.Data.CommandWord,
                                         sizeof(curr_rec.Data.CommandWord)) != 0;

                           if (!diff)
                           {
                              diff = curr_rec.Data.StatusWord != new_rec.Data.StatusWord;
                           }

                           if (!diff)
                           {
                              for (int x = 0; x < 32; x++)
                              {
                                 if (curr_rec.Data.Words[x] != new_rec.Data.Words[x])
                                 {
                                    diff = true;
                                    break;
                                 }
                              }
                           }

                           if (diff)
                           {
                              idx = k;
                              curr_time = records[i][j][idx].Time;
                              break;
                           }
                        }
                     }
                  }
                  ImGui::SameLine();
                  if (ImGui::Button("Next Diff"))
                  {
                     if (idx < (int)records[i][j].size() - 1)
                     {
                        int start_idx = idx;

                        if (start_idx == -1)
                           start_idx = 0;

                        for (int k = start_idx; k < records[i][j].size(); k++)
                        {
                           bool diff = false;
                           const auto& curr_rec = records[i][j][start_idx];
                           const auto& new_rec = records[i][j][k];

                           diff = memcmp(&curr_rec.Data.CommandWord,
                                         &new_rec.Data.CommandWord,
                                         sizeof(curr_rec.Data.CommandWord)) != 0;

                           if (!diff)
                           {
                              diff = curr_rec.Data.StatusWord != new_rec.Data.StatusWord;
                           }

                           if (!diff)
                           {
                              for (int x = 0; x < 32; x++)
                              {
                                 if (curr_rec.Data.Words[x] != new_rec.Data.Words[x])
                                 {
                                    diff = true;
                                    break;
                                 }
                              }
                           }

                           if (diff)
                           {
                              idx = k;
                              curr_time = records[i][j][idx].Time;
                              break;
                           }
                        }
                     }
                  }

                  if (idx >= 0 && idx < records[i][j].size())
                  {
                     ImGui::Text("Index: %d", idx);
                     if (records[i][j].size())
                     {
                        const auto& r = records[i][j][idx];
                        int num_words = r.Data.CommandWord.WordCount;
                        if (num_words == 0)
                           num_words = 32;
                        ImGui::Text("Record:  %d", r.Record);
                        ImGui::Text("Time:    %f", r.Time);
                        ImGui::Text("Channel: %d", r.Channel);
                        ImGui::Text("Bus:     %d", r.Bus);

                        bool diff = false;
                        if (idx > 0)
                        {
                           diff = memcmp(&r.Data.CommandWord,
                                         &records[i][j][idx-1].Data.CommandWord,
                                         sizeof(r.Data.CommandWord)) != 0;
                        }

                        if (diff)
                           ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                        ImGui::Text("Cmd:     0x%04x (%d, %c, %d, %d)",
                                    r.Data.CommandWord,
                                    r.Data.CommandWord.RemoteTerminal,
                                    r.Data.CommandWord.Transmit ? 'T' : 'R',
                                    r.Data.CommandWord.Subaddress,
                                    r.Data.CommandWord.WordCount);
                        if (diff)
                           ImGui::PopStyleColor();

                        diff = false;
                        if (idx > 0)
                           diff = r.Data.StatusWord != records[i][j][idx-1].Data.StatusWord;

                        if (diff)
                           ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                        ImGui::Text("Status:  0x%04x", r.Data.StatusWord);
                        if (diff)
                           ImGui::PopStyleColor();

                        ImGui::Text("Data Words:");
                        for (int k = 0; k < num_words; k++)
                        {
                           diff = false;
                           if (idx > 0)
                              diff = r.Data.Words[k] != records[i][j][idx-1].Data.Words[k];

                           if (diff)
                              ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                           ImGui::Text("0x%04x  ", r.Data.Words[k]);
                           if (diff)
                              ImGui::PopStyleColor();

                           if (k < num_words - 1 && ((k + 1) % 8 != 0))
                              ImGui::SameLine();
                        }
                     }
                  }

                  ImGui::TreePop();
               }
            }
         }
      }

      ImGui::EndChild();

      ImGui::End();

      // Render ImGui
      ImGui::Render();
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

      glfwSwapBuffers(window);

      // wait until next frame
      std::this_thread::sleep_until(frame_time);
      frame_time += framerate{1};
   }

   return 0;
}
