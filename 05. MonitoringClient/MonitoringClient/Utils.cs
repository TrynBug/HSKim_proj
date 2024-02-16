using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MonitoringClient
{
    internal class Utils
    {
        public static int GetFirstDigit(int value)
        {
            int number = Math.Abs(value);
            if (number < 10) return number;
            else if (number < 100) return number / 10;
            else if (number < 1000) return number / 100;
            else if (number < 10000) return number / 1000;
            else if (number < 100000) return number / 10000;
            else if (number < 1000000) return number / 100000;
            else if (number < 10000000) return number / 1000000;
            else if (number < 100000000) return number / 10000000;
            else if (number < 1000000000) return number / 100000000;
            else return number / 1000000000;
        }

        public static int GetNumDigit(int value)
        {
            int number = Math.Abs(value);
            if (number < 10) return 1;
            else if (number < 100) return 2;
            else if (number < 1000) return 3;
            else if (number < 10000) return 4;
            else if (number < 100000) return 5;
            else if (number < 1000000) return 6;
            else if (number < 10000000) return 7;
            else if (number < 100000000) return 8;
            else if (number < 1000000000) return 9;
            else return 10;
        }

        public static int CeilOfSameDigit(int number)
        {
            if (number < 0)
            {
                return FloorOfSameDigit(Math.Abs(number)) * -1;
            }

            int firstDigit;
            if (number < 10)
            {
                firstDigit = number;
                return firstDigit + 1;
            }
            else if (number < 100)
            {
                firstDigit = number / 10;
                return (firstDigit + 1) * 10;
            }
            else if (number < 1000)
            {
                firstDigit = number / 100;
                return (firstDigit + 1) * 100;
            }
            else if (number < 10000)
            {
                firstDigit = number / 1000;
                return (firstDigit + 1) * 1000;
            }
            else if (number < 100000)
            {
                firstDigit = number / 10000;
                return (firstDigit + 1) * 10000;
            }
            else if (number < 1000000)
            {
                firstDigit = number / 100000;
                return (firstDigit + 1) * 100000;
            }
            else if (number < 10000000)
            {
                firstDigit = number / 1000000;
                return (firstDigit + 1) * 1000000;
            }
            else if (number < 100000000)
            {
                firstDigit = number / 10000000;
                return (firstDigit + 1) * 10000000;
            }
            else if (number < 1000000000)
            {
                firstDigit = number / 100000000;
                return (firstDigit + 1) * 100000000;
            }
            else
            {
                firstDigit = number / 1000000000;
                return Math.Min(int.MaxValue, (firstDigit + 1) * 1000000000);
            }
        }

        public static int FloorOfSameDigit(int number)
        {
            if(number < 0)
            {
                return CeilOfSameDigit(Math.Abs(number)) * -1;
            }

            int firstDigit;
            if(number == 0)
            {
                return 0;
            }
            else if (number < 10)
            {
                return number - 1;
            }
            else if (number < 100)
            {
                firstDigit = number / 10;
                return firstDigit * 10;
            }
            else if (number < 1000)
            {
                firstDigit = number / 100;
                return firstDigit * 100;
            }
            else if (number < 10000)
            {
                firstDigit = number / 1000;
                return firstDigit * 1000;
            }
            else if (number < 100000)
            {
                firstDigit = number / 10000;
                return firstDigit * 10000;
            }
            else if (number < 1000000)
            {
                firstDigit = number / 100000;
                return firstDigit * 100000;
            }
            else if (number < 10000000)
            {
                firstDigit = number / 1000000;
                return firstDigit * 1000000;
            }
            else if (number < 100000000)
            {
                firstDigit = number / 10000000;
                return firstDigit * 10000000;
            }
            else if (number < 1000000000)
            {
                firstDigit = number / 100000000;
                return firstDigit * 100000000;
            }
            else
            {
                firstDigit = number / 1000000000;
                return Math.Min(int.MaxValue, firstDigit * 1000000000);
            }
        }
    }
}
