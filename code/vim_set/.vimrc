colo elflord
set ic
set nobackup
set nocompatible
set wrap
set mouse=a
"colo evening
set nu
set autoindent
set expandtab
set tabstop=4
set shiftwidth=4
set hlsearch
"逐步搜索模式，对当前键入的字符进行搜索而不必等待键入完成
set incsearch
" space 折叠
set foldenable
set foldmethod=syntax
set foldlevel=100
nnoremap <space> @=((foldclosed(line('.'))<0)?'zc':'zo')<CR>

set cursorline
hi CursorLine  cterm=NONE ctermbg=DarkGray
"ctermfg=white
"hi CursorColumn cterm=NONE ctermbg=darkred ctermfg=white

map <C-F12> :!ctags -R --c++-kinds=+p --fields=+iaS --extra=+q .<CR>  
map ct :!ctags -R --c++-kinds=+p --fields=+iaS --extra=+q .<CR>  
map cs :!cscope -Rbq<CR>  

map cm :set mouse=a<CR>  
map cn :set mouse=<CR>  

let Tlist_Show_One_File=1  
let Tlist_Exit_OnlyWindow=1 

set nocp  
filetype plugin on 

set cscopequickfix=s-,c-,d-,i-,t-,e-

"--------
let g:miniBufExplMapWindowNavVim = 1   
let g:miniBufExplMapWindowNavArrows = 1   
let g:miniBufExplMapCTabSwitchBufs = 1   
let g:miniBufExplModSelTarget = 1  
let g:miniBufExplMoreThanOne=0  

"let g:miniBufExplMinSize = 9

let g:winManagerWidth = 50

let g:NERDTree_title="[NERDTree]"  
let g:winManagerWindowLayout="NERDTree|TagList"  
  
function! NERDTree_Start()  
	exec 'NERDTree'  
endfunction  
	    
function! NERDTree_IsValid()  
	return 1  
endfunction  
			  
nmap wm :WMToggle<CR>  

"cscope hot key
nmap <C-_>s :cs find s <C-R>=expand("<cword>")<CR><CR>
nmap <C-_>g :cs find g <C-R>=expand("<cword>")<CR><CR>
nmap <C-_>c :cs find c <C-R>=expand("<cword>")<CR><CR>
nmap <C-_>t :cs find t <C-R>=expand("<cword>")<CR><CR>
nmap <C-_>e :cs find e <C-R>=expand("<cword>")<CR><CR>
nmap <C-_>f :cs find f <C-R>=expand("<cfile>")<CR><CR>
nmap <C-_>i :cs find i <C-R>=expand("<cfile>")<CR><CR>
nmap <C-_>d :cs find d <C-R>=expand("<cword>")<CR><CR>

" QuickFix hot key
nmap <F6> :cp<cr>
nmap <F7> :cn<cr>

" transfer/read and write one block of text between vim sessions
" Usage:
" `from' session:
" ma
" move to end-of-block
" xw
"
" `to' session:
" move to where I want block inserted
" xr
"
if has("unix")
    nmap xr :r $HOME/.vimxfer<CR>
    nmap xw :'a,.w! $HOME/.vimxfer<CR>
    vmap xr c<esc>:r $HOME/.vimxfer<CR>
    vmap xw :w! $HOME/.vimxfer<CR>
else
    nmap xr :r c:/.vimxfer<CR>
    nmap xw :'a,.w! c:/.vimxfer<CR>
    vmap xr c<esc>:r c:/.vimxfer<cr>
    vmap xw :w! c:/.vimxfer<CR>
endif

let g:DoxygenToolkit_briefTag_pre="@Synopsis " 
let g:DoxygenToolkit_paramTag_pre="@Param " 
let g:DoxygenToolkit_returnTag="@Returns " 
let g:DoxygenToolkit_blockHeader="================================================================================ "
let g:DoxygenToolkit_blockFooter="================================================================================ " 
"let g:DoxygenToolkit_authorName="langyj.NightStark " 

let g:DoxygenToolkit_authorName="langyj, lyj051031448@163.com"
"let s:licenseTag = "\<enter>Copyright(C)\<enter>"
"let s:licenseTag = s:licenseTag . "For free\<enter>"
"let s:licenseTag = s:licenseTag . "All right reserved"
"let g:DoxygenToolkit_licenseTag="Copyright(c) All Right Reserver "
"let g:DoxygenToolkit_licenseTag = s:licenseTag
let g:DoxygenToolkit_briefTag_funcName="yes"
let g:doxygen_enhanced_color=1


