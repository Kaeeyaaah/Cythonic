# Check if an input file was provided as an argument
if ($args.Count -eq 0) {
    Write-Host "Error: No input file provided."
    Write-Host "Usage: ./run.ps1 <path-to-cytho-file>"
    exit 1
}

$inputFile = $args[0]

# Compile Lexer
Write-Host "Compiling Lexer..."
gcc Lexer/Lexer.c -o cythonic-lexer.exe -DLEXER_MAIN
if ($LASTEXITCODE -ne 0) { exit 1 }

# Compile Parser
Write-Host "Compiling Parser..."
gcc Parser/Parser.c Lexer/Lexer.c -o cythonic-parser.exe
if ($LASTEXITCODE -ne 0) { exit 1 }

# Run Lexer
Write-Host "Running Lexer on $inputFile..."
if (-not (Test-Path $inputFile)) {
    Write-Host "Error: Input file '$inputFile' not found."
    exit 1
}

./cythonic-lexer.exe $inputFile
if ($LASTEXITCODE -ne 0) { exit 1 }

# Run Parser
# Assuming the lexer creates a .symboltable.txt file based on the input filename
$symbolTableFile = "$inputFile.symboltable.txt"

if (-not (Test-Path $symbolTableFile)) {
    Write-Host "Error: Symbol table file '$symbolTableFile' was not generated."
    exit 1
}

Write-Host "Running Parser on $symbolTableFile..."
./cythonic-parser.exe $symbolTableFile