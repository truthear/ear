
rm -rf tmp
mkdir tmp
cd tmp
pdflatex ../HDD.tex
cp HDD.pdf ../
cd  ..
rm -rf tmp