using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;

namespace DummyClient.Data
{
    public static class Config
    {
        /* room */
        public const int FPS = 20;

        /* map */
        public const int CellMultiple = 2;

        /* float */
        public const float DifferenceTolerance = 0.001f;

        /* dummy client */
        public const int MaxNumberOfClient = 100;
        public const int MaxConnectionCountPerFrame = 5;
        public const float DisconnectProb = 0.0001f;


        //private static string _toString = string.Empty;
        //public static new string ToString()
        //{
        //    if(_toString == string.Empty)
        //    {


        //        Type type = typeof(Config);
        //        MemberInfo[] members = type.GetMembers();
        //        foreach(MemberInfo member in members)
        //        {
        //            object obj = new object();
        //            switch(member.MemberType)
        //            {
        //                case MemberTypes.Field:
        //                    FieldInfo field = (FieldInfo)member;
        //                    _toString += $"    {field.Name} : {FieldToString(field)}\n";
        //                    break;
        //                case MemberTypes.Property:
        //                    PropertyInfo property = (PropertyInfo)member;
        //                    property.GetValue(obj);
        //                    _toString += $"    {property.Name} : {property}\n";
        //                    break;
        //            }
        //        }
        //    }

        //    return _toString;
        //}


        //private static string FieldToString(FieldInfo field)
        //{
        //    Type type = field.FieldType;
        //    if(type == typeof(System.Int32))
        //    {
        //        System.Int32 val = new System.Int32();
        //        field.GetValue(val);
        //        return val.ToString();
        //    }
        //    return "";
        //}
    }
}
