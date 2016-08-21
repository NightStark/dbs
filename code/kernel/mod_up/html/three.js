function three() {
	layer.ready(function(){ 
		layer.open({
		  type: 2,
		  title: false,
		  closeBtn: 0, //不显示关闭按钮
		  shade: [0],
		  area: ['340px', '215px'],
		  offset: 'rb', //右下角弹出
		  time: 2000, //2秒后自动关闭
		  shift: 2,
		  content: ['./rb.html', 'no'], //iframe的url，no代表不显示滚动条
		  end: function(){ //此处用于演示
			layer.open({
			  type: 2,
			  title: '很多时候，我们想最大化看，比如像这个页面。',
			  shadeClose: true,
			  shade: false,
			  maxmin: true, //开启最大化最小化按钮
			  area: ['893px', '600px'],
			  content: 'show.html'
			});
		  }
		});
	});	
}

$(document).ready(function(){  
    three();  
});