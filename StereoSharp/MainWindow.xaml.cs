using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using SharpGL;
using System.Numerics;
using SharpGL.Enumerations;
using System.Diagnostics;
using System.Threading;
using System.Runtime.InteropServices;

namespace StereoSharp
{
    using static Math;
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
        }

        private Line[] lines;
        //private Stopwatch stopwatch = Stopwatch.StartNew();
        //private double delta;

        private void Test(OpenGL gl)
        {
            string vertexShaderSource = $@"
        #version 330 core
        layout (location = 0) in vec3 aPos;
        void main()
        {{
            gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
        }}\0";
            string fragmentShaderSource = $@"
        #version 330 core
        out vec4 FragColor;
        void main()
        {{
        	FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
        }}\0";

            var success = new int[1];
            var infoLog = new StringBuilder();

            var vertexShader = gl.CreateShader(OpenGL.GL_VERTEX_SHADER);
            gl.ShaderSource(vertexShader, vertexShaderSource);
            gl.CompileShader(vertexShader);

            //test
            gl.GetShader(vertexShader, OpenGL.GL_COMPILE_STATUS, success);
            if (success[0] != 0)
            {
                gl.GetShaderInfoLog(vertexShader, 512, IntPtr.Zero, infoLog);
                Console.WriteLine("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" + infoLog.ToString());
            }
            ////////////////
            var fragmentShader = gl.CreateShader(OpenGL.GL_FRAGMENT_SHADER);
            gl.ShaderSource(fragmentShader, fragmentShaderSource);
            gl.CompileShader(fragmentShader);

            //test
            gl.GetShader(fragmentShader, OpenGL.GL_COMPILE_STATUS, success);
            if (success[0] != 0)
            {
                gl.GetShaderInfoLog(fragmentShader, 512, IntPtr.Zero, infoLog);
                Console.WriteLine("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" + infoLog.ToString());
            }


            var shaderProgram = gl.CreateProgram();

            gl.AttachShader(shaderProgram, vertexShader);
            gl.AttachShader(shaderProgram, fragmentShader);
            gl.LinkProgram(shaderProgram);

            //test
            gl.GetProgram(shaderProgram, OpenGL.GL_LINK_STATUS, success);
            if (success[0] != 0)
            {
                gl.GetProgramInfoLog(shaderProgram, 512, IntPtr.Zero, infoLog);
                Console.WriteLine("ERROR::PROGRAM::LINKING_FAILED\n" + infoLog.ToString());
            }

        }

        private void OpenGLControl_OpenGLDraw1(object sender, SharpGL.SceneGraph.OpenGLEventArgs args)
        {

            //Console.WriteLine(delta);


            var gl = args.OpenGL;
            //gl.LoadIdentity();
            string vertexShaderSource = $@"
#version 330 core
layout (location = 0) in vec3 aPos;
void main()
{{
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}}" + '\0';
            string fragmentShaderSource = $@"
#version 330 core
out vec4 FragColor;
void main()
{{
        	FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);

}}" + '\0';

            //string vertexShaderSource = "#version 330 core\r\nlayout (location = 0) in vec3 aPos;\r\nvoid main()\r\n{\r\ngl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\r\n}\r\n\0";
            //string fragmentShaderSource = "#version 330 core\nout vec4 FragColor;void main(){FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);}\n\0";

            var success = new int[1];
            var infoLog = new StringBuilder();

            var vertexShader = gl.CreateShader(OpenGL.GL_VERTEX_SHADER);
            gl.ShaderSource(vertexShader, vertexShaderSource);
            gl.CompileShader(vertexShader);

            //test
            gl.GetShader(vertexShader, OpenGL.GL_COMPILE_STATUS, success);
            if (success[0] != OpenGL.GL_TRUE)
            {
                gl.GetShaderInfoLog(vertexShader, 512, IntPtr.Zero, infoLog);
                Console.WriteLine("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" + infoLog.ToString());
            }
            ////////////////
            var fragmentShader = gl.CreateShader(OpenGL.GL_FRAGMENT_SHADER);
            gl.ShaderSource(fragmentShader, fragmentShaderSource);
            gl.CompileShader(fragmentShader);

            //test
            gl.GetShader(fragmentShader, OpenGL.GL_COMPILE_STATUS, success);
            if (success[0] != OpenGL.GL_TRUE)
            {
                gl.GetShaderInfoLog(fragmentShader, 512, IntPtr.Zero, infoLog);
                Console.WriteLine("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" + infoLog.ToString());
            }


            var shaderProgram = gl.CreateProgram();

            gl.AttachShader(shaderProgram, vertexShader);
            gl.AttachShader(shaderProgram, fragmentShader);
            gl.LinkProgram(shaderProgram);

            //test
            gl.GetProgram(shaderProgram, OpenGL.GL_LINK_STATUS, success);
            if (success[0] != OpenGL.GL_TRUE)
            {
                gl.GetProgramInfoLog(shaderProgram, 512, IntPtr.Zero, infoLog);
                Console.WriteLine("ERROR::PROGRAM::LINKING_FAILED\n" + infoLog.ToString());
            }

            //we don't need shaders anymore
            gl.DeleteShader(vertexShader);
            gl.DeleteShader(fragmentShader);
            //Test(gl);
            gl.ClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            gl.Clear(OpenGL.GL_COLOR_BUFFER_BIT | OpenGL.GL_DEPTH_BUFFER_BIT);
            gl.PointSize(2f);

            var vbo = new uint[1];
            gl.GenBuffers(1, vbo);
            var vao = new uint[1];
            gl.GenVertexArrays(1, vao);



            gl.MatrixMode(MatrixMode.Projection);
            //gl.LoadIdentity();
            //gl.Ortho(0, ((FrameworkElement)sender).ActualWidth, ((FrameworkElement)sender).ActualHeight, 0, -10, 10);
            gl.Ortho(-1, 1, -1, 1, -10, 10);

            //  Back to the modelview.
            gl.MatrixMode(MatrixMode.Modelview);




            var vertices = new float[]
            {
                -0.5f, -0.1f, 0f, // right  
        		1f,  0.8f, 0f  // top   
            };

            //var vertices = new float[]
            //{
            //            50.5f, 50.5f, 0.1f, // right  
            //      15.9f,  150.9f, 0.1f  // top   
            //};
            gl.BindBuffer(OpenGL.GL_ARRAY_BUFFER, vbo[0]);
            gl.BufferData(OpenGL.GL_ARRAY_BUFFER, vertices, OpenGL.GL_STREAM_DRAW);
            gl.VertexAttribPointer(OpenGL.GL_POINTS, 3, OpenGL.GL_FLOAT, false, 3 * sizeof(float), IntPtr.Zero);
            gl.EnableVertexAttribArray(OpenGL.GL_POINTS);

            // unbind VBO
            gl.BindBuffer(OpenGL.GL_ARRAY_BUFFER, 0);

            gl.UseProgram(shaderProgram);
            //gl.BindVertexArray(vao[0]);
            gl.DrawArrays(OpenGL.GL_LINES, 0, 2);
            //gl.Flush();
        }


        private bool isFirst = true;
        private void OpenGLControl_OpenGLDraw(object sender, SharpGL.SceneGraph.OpenGLEventArgs args)
        {

            fps.Content = view.FrameRate;
            var gl = args.OpenGL;

            gl.ClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            gl.Clear(OpenGL.GL_COLOR_BUFFER_BIT | OpenGL.GL_DEPTH_BUFFER_BIT);

            gl.PointSize(2);

            foreach (var line in lines)
            {
                var vertices = line.Vertices;


                gl.BindBuffer(OpenGL.GL_ARRAY_BUFFER, line.VBO);
                gl.BufferData(OpenGL.GL_ARRAY_BUFFER, vertices, OpenGL.GL_DYNAMIC_DRAW);

                //gl.BufferData(OpenGL.GL_ARRAY_BUFFER, vertices, OpenGL.GL_STREAM_DRAW);
                gl.VertexAttribPointer(OpenGL.GL_POINTS, 3, OpenGL.GL_FLOAT, false, 3 * sizeof(float), IntPtr.Zero);
                gl.EnableVertexAttribArray(OpenGL.GL_POINTS);

                // unbind VBO
                gl.BindBuffer(OpenGL.GL_ARRAY_BUFFER, 0);

                gl.UseProgram(line.ShaderProgram);
                gl.BindVertexArray(line.VAO);
                gl.DrawArrays(OpenGL.GL_LINES, 0, 2);

                // unbind VAO
                //gl.BindVertexArray(0);
            }


            //gl.Flush();

            MoveLines();
            isFirst = false;
        }

        private void MoveLines()
        {
            fps.Content = 1000 / view.DeltaTime;
           
            var speed = view.DeltaTime * 0.01;
            var r = 1.5;

            foreach (var line in lines)
            {
                line.End.X = (float)(Sin(line.T) * r);
                line.End.Y = (float)(Cos(line.T) * r);

                line.T += 0.1 * speed;
            }
        }

        private void OpenGLControl_OpenGLInitialized(object sender, SharpGL.SceneGraph.OpenGLEventArgs args)
        {
            //view.FrameRate = 60;

            var gl = args.OpenGL;

            gl.MatrixMode(MatrixMode.Projection);
            //gl.LoadIdentity();
            //gl.Ortho(0, ((FrameworkElement)sender).ActualWidth, ((FrameworkElement)sender).ActualHeight, 0, -10, 10);
            gl.Ortho(-1, 1, -1, 1, -10, 10);
            //  Back to the modelview.
            gl.MatrixMode(MatrixMode.Modelview);

            lines = CreateLines(gl).ToArray();


        }

        private uint CreateShaderProgram(OpenGL gl)
        {
            var rnd = new Random();

            System.Globalization.CultureInfo customCulture = (System.Globalization.CultureInfo)System.Threading.Thread.CurrentThread.CurrentCulture.Clone();
            customCulture.NumberFormat.NumberDecimalSeparator = ".";

            System.Threading.Thread.CurrentThread.CurrentCulture = customCulture;

            string vertexShaderSource = $@"
#version 330 core
layout (location = 0) in vec3 aPos;
void main()
{{
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}}" + '\0';
            string fragmentShaderSource = $@"
#version 330 core
out vec4 FragColor;
void main()
{{
    FragColor = vec4({rnd.NextDouble():0.00}f, {rnd.NextDouble():0.00}f, {rnd.NextDouble():0.00}f, 1.0f);
}}" + '\0';
        	//FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);

            var success = new int[1];
            var infoLog = new StringBuilder();

            var vertexShader = gl.CreateShader(OpenGL.GL_VERTEX_SHADER);
            gl.ShaderSource(vertexShader, vertexShaderSource);
            gl.CompileShader(vertexShader);

            //test
            gl.GetShader(vertexShader, OpenGL.GL_COMPILE_STATUS, success);
            if (success[0] != OpenGL.GL_TRUE)
            {
                gl.GetShaderInfoLog(vertexShader, 512, IntPtr.Zero, infoLog);
                Console.WriteLine("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" + infoLog.ToString());
            }
            ////////////////
            var fragmentShader = gl.CreateShader(OpenGL.GL_FRAGMENT_SHADER);
            gl.ShaderSource(fragmentShader, fragmentShaderSource);
            gl.CompileShader(fragmentShader);

            //test
            gl.GetShader(fragmentShader, OpenGL.GL_COMPILE_STATUS, success);
            if (success[0] != OpenGL.GL_TRUE)
            {
                gl.GetShaderInfoLog(fragmentShader, 512, IntPtr.Zero, infoLog);
                Console.WriteLine("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" + infoLog.ToString());
            }


            var shaderProgram = gl.CreateProgram();

            gl.AttachShader(shaderProgram, vertexShader);
            gl.AttachShader(shaderProgram, fragmentShader);
            gl.LinkProgram(shaderProgram);

            //test
            gl.GetProgram(shaderProgram, OpenGL.GL_LINK_STATUS, success);
            if (success[0] != OpenGL.GL_TRUE)
            {
                gl.GetProgramInfoLog(shaderProgram, 512, IntPtr.Zero, infoLog);
                Console.WriteLine("ERROR::SHADER::PROGRAM::LINKING_FAILED\n" + infoLog.ToString());
            }

            // we don't need shaders anymore
            //gl.DeleteShader(vertexShader);
            //gl.DeleteShader(fragmentShader);

            return shaderProgram;
        }

        private IEnumerable<Line> CreateLines(OpenGL gl)
        {
            //var width = view.ActualWidth;
            //var height = view.ActualHeight;

            var lineCount = 1000;


            var lines = new List<Line>();

            var r = 1;

            for (int i = 0; i < lineCount; i++)
            {
                var t = (double)i / lineCount * 3.1415926 * 2;

                uint Get(Action<int, uint[]> func)
                {
                    var arr = new uint[1];
                    func(1, arr);
                    return arr[0];
                }

                var line = new Line
                {
                    //Start = new Vector3(),
                    //End = new Vector3(
                    //    (float)(r * Sin(t)),
                    //    (float)(r * Cos(t)),
                    //    0
                    //),
                    End = new Vector3(),
                    Start = new Vector3(
                        (float)(r * Sin(t)),
                        (float)(r * Cos(t)),
                        0
                    ),
                    T = t,
                    //ShaderProgram = 0,
                    ShaderProgram = CreateShaderProgram(gl),
                    VAO = Get(gl.GenVertexArrays),
                    VBO = Get(gl.GenBuffers),
                };

                //// bake vertex array
                //gl.BindVertexArray(line.VAO);
                //gl.VertexAttribPointer(OpenGL.GL_POINTS, 3, OpenGL.GL_FLOAT, false, 3 * sizeof(float), IntPtr.Zero);


                lines.Add(line);
            }

            return lines;
        }

        private void View_Resized(object sender, SharpGL.SceneGraph.OpenGLEventArgs args)
        {
            //  Get the OpenGL instance.
            //var gl = args.OpenGL;

            ////  Create an orthographic projection.
            //gl.MatrixMode(MatrixMode.Projection);
            //gl.LoadIdentity();
            //gl.Ortho(0, ((FrameworkElement)sender).ActualWidth, ((FrameworkElement)sender).ActualHeight, 0, -10, 10);

            ////  Back to the modelview.
            //gl.MatrixMode(MatrixMode.Modelview);
        }


        private void View_MouseDown(object sender, MouseButtonEventArgs e)
        {

        }
    }

    public class Line : IDisposable
    {
        public float[] Vertices =>
            new float[] { Start.X, Start.Y, Start.Z, End.X, End.Y, End.Z };

        public IntPtr Points;

        public Vector3 Start;
        public Vector3 End;

        public uint VBO;
        public uint VAO;

        public double T;

        public uint ShaderProgram;

        public void ApplyPoints()
        {
            var v = Vertices;
            Points = Marshal.AllocHGlobal(v.Length * sizeof(float));
            Marshal.Copy(v, 0, Points, v.Length);
        }

        public void Dispose()
        {

            Marshal.FreeHGlobal(Points);
        }
    }
    
}
