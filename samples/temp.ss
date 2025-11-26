#函数定义方式一
abs(num)={
   return if(num>0){num}
          else{-num}
}
#函数定义方式二
var abs=func(num){
      return if(num>0){num}
          else{-num} 
}
#函数定义方式三
func abs(num){
     return if(num>0){num}
          else{-num}  
}
@print(@abs(-4))