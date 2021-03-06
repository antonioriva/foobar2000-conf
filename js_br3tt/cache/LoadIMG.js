var fso = new ActiveXObject("Scripting.FileSystemObject");
var Img = new ActiveXObject("WIA.ImageFile.1");
var IP = new ActiveXObject("WIA.ImageProcess.1");
IP.Filters.Add(IP.FilterInfos("Scale").FilterID);//ID = 1
IP.Filters.Add(IP.FilterInfos("Crop").FilterID);//ID = 2
IP.Filters.Add(IP.FilterInfos("Convert").FilterID);//ID = 3
function resize_image(path,crc,tranparent)
{
    var ratio = 1;
    var cachesize = 200;
    var img_w = cachesize, img_h = cachesize, cr_x = 0, cr_y = 0;
    try{
    Img.LoadFile(path);
    }catch(err){
		return false;
    }
if(Img.Height >= Img.Width) {
    ratio = Img.Width / Img.Height;
    img_w = img_w * ratio;
    cr_x = (img_h - img_w)/2;
} else {
    ratio = Img.Height / Img.Width;
    img_h = img_h * ratio;
    cr_y = (img_w - img_h)/2;
}
    IP.Filters(1).Properties("MaximumWidth") = img_w;
    IP.Filters(1).Properties("MaximumHeight") = img_h;
    if(tranparent == "true"){
        IP.Filters(3).Properties("FormatID").Value = '{B96B3CAF-0728-11D3-9D7B-0000F81EF32E}';
    }else{
        IP.Filters(3).Properties("FormatID").Value = '{B96B3CAE-0728-11D3-9D7B-0000F81EF32E}';
        IP.Filters(3).Properties("Quality").Value = 95; 
    }
    //IP.Filters(2).Properties("Left") = cr_x;
    //IP.Filters(2).Properties("Top") = cr_y;
    //IP.Filters(2).Properties("Right") = cr_x;
    //IP.Filters(2).Properties("Bottom") = cr_y;
    Img = IP.Apply(Img);
    try{
        if(fso.FileExists(WScript.arguments(0) + "\\js_br3tt\\cache\\imgcache\\" + crc))
            fso.DeleteFile(WScript.arguments(0)+ "\\js_br3tt\\cache\\imgcache\\" + crc);
        Img.SaveFile(WScript.arguments(0) + "\\js_br3tt\\cache\\imgcache\\" + crc);
    }catch(err){
		return false;
    }
	return true;
}
resize_image(WScript.arguments(1),WScript.arguments(2),WScript.arguments(3));
